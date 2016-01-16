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

static char tohex(char high, char low)
{
	high -= 48;
	low -= 48;
	if (high > 9)
		high -= 7;
	if (low > 9)
		low -= 7;
	return high * 16 + low;
};

char *geturl(char *header)
{
	char *ptstart, *ptend;
	char *ret, *write;

	ptstart = index(header, ' ') + 1; /* this points to the first slash */
	ptend = strtok(ptstart, " ?"); /* this points to the end of the thing */

	write = ret = calloc(ptend - ptstart + 1, sizeof(char));
	while (ptstart != ptend) {
		if (*ptstart == '%') {
			ptstart++;
			char low, high;
			high = *ptstart++;
			low  = *ptstart;
			*write = tohex(high, low);
		} else {
			*write = *ptstart;
		}
		ptstart ++;
		write ++;
	}
	*write = '\0';
	return ret;
}

int handle(int newsockfd, struct sockaddr_in socket, socklen_t socklen)
{
	char buffer[256], path[PATH_MAX], *url;
	struct stat path_stat;
	int n;

	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0)
		error("error reading");

	url = geturl(buffer);

	printf("[%s] %s\n", inet_ntoa(socket.sin_addr), url);

	snprintf(path, PATH_MAX, "%s/%s", basedir, url);

	while (n == 255)
		n = read(newsockfd, buffer, 255);

	if (stat(path, &path_stat)) {
		write(newsockfd, "HTTP/1.1 404 Not Found\n", 23);
	} else {
		if (S_ISDIR(path_stat.st_mode)) {
			handle_directory(url, path, newsockfd);
		} else {
			handle_file(url, path, newsockfd);
		}
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
	int sleeptime = 10;

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

	if (getuid())
		portno = 8080;
	else
		portno = 80;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("error socketing");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	info("Serving in http://%s:%d/", "127.0.0.1", ntohs(serv_addr.sin_port));

	while ((err = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))) {
		perror("error binding");
		printf("Trying again in %d seconds\n", sleeptime);
		sleep(sleeptime);
	}

	info("Socket binded\n");

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
			handle(newsockfd, cli_addr, clilen);
			exit(EXIT_SUCCESS);
			break;
		default:
			close(newsockfd);
		}
	}

	close(sockfd);

	return 0;
}
