#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "handle_directory.h"
#include "handle_file.h"
#include "util.h"

int sockfd = -1;
char basedir[PATH_MAX];

char *geturl(char *header)
{
	char *ptstart, *ptend;
	char *ret;

	ptstart = index(header, ' ') + 1;
	ptend = index(ptstart, ' ');

	ret = calloc(128, sizeof(char));
	strncpy(ret, ptstart, ptend - ptstart);
	ret[strlen(ret)] = '\0';
	return ret;
}

int handle(int newsockfd)
{
	char buffer[256], path[PATH_MAX], *url;
	struct stat path_stat;
	int n;

	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0)
		error("error reading");

	url = geturl(buffer);
	snprintf(path, PATH_MAX, "%s/%s", basedir, url);

	while (n == 255)
		n = read(newsockfd, buffer, 255);

	stat(path, &path_stat);

	if (S_ISDIR(path_stat.st_mode)) {
		handle_directory(url, path, newsockfd);
	} else{
		handle_file(url, path, newsockfd);
	}

	close(newsockfd);
	free(url);
	return 0;
}

void waitforit(int sig)
{
	int a;

	wait(&a);
}

void abandonship(int sig)
{
	printf("[%d childof %d] %s\n", getpid(), getppid(),
	       "KILLSIGNAL received, quitting...");
	fflush(stdout);
	if (sockfd != -1)
		close(sockfd);
	sockfd = -1;
}

int main(int argc, char *argv[])
{
	int newsockfd, portno;
	socklen_t clilen;

	struct sockaddr_in serv_addr, cli_addr;
	int err;

	if (argc < 2) {
		printf("Please provide a directory to serve\n");
		exit(EXIT_FAILURE);
	};

	if (!realpath(argv[1], basedir)) {
		printf("Folder not found!\n");
		exit(EXIT_FAILURE);
	}

	info("Serving directory: %s", basedir);

	signal(SIGINT, &abandonship);
	signal(SIGCHLD, &waitforit);
	portno = 8080;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("error socketing");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	err = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (err < 0)
		error("error binding");

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1) {
		newsockfd =
		    accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			error("error accepting");

		switch (fork()) {
		case -1:
			error("error forking");
			break;
		case 0:
			close(sockfd);
			handle(newsockfd);
			exit(EXIT_SUCCESS);
			break;
		default:
			close(newsockfd);
		}
	}

	close(sockfd);

	return 0;
}
