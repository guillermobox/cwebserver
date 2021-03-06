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

#include <time.h>
void handle_notfound(int fdout)
{
	time_t tnow;
	struct string headers = STRING_EMPTY;

	do {
		tnow = time(NULL);
		stringf(&headers, "Content-type: %s\n", "text/plain");
		stringf(&headers, "Content-length: %ld\n", 10);
		stringf(&headers, "Date: %s\n", ctime(&tnow));

		if (write(fdout, "HTTP/1.1 404 Not Found\n", 23) != 23) {
			break;
		};
		if (write(fdout, headers.content, headers.length) != (ssize_t) headers.length) {
			break;
		};
		if (write(fdout, "Not found\n", 10) != 10){
			break;
		};
		free(headers.content);
		return;
	} while (0);
	error("error writing on socket the 404 page");
}

void handle_redirection(const char *url, const char *newurl, int fdout)
{
	time_t tnow;
	struct string response = STRING_EMPTY;
	(void) url;

	tnow = time(NULL);
	stringf(&response, "HTTP/1.1 301 Moved Permanently\n");
	stringf(&response, "Location: /%s\n", newurl + 1);
	stringf(&response, "Date: %s\n", ctime(&tnow));

	if (write(fdout, response.content, response.length) != (ssize_t) response.length) {
		error("error writing on file descriptor");
	};

	free(response.content);
}

static char tohex(char high, char low)
{
	high -= 48;
	low -= 48;
	if (high > 9)
		high -= 7;
	if (low > 9)
		low -= 7;
	return high * 16 + low;
}

char *geturl(char *header)
{
	char *end, *inpt, *oupt, *ret;
	char low, high;

	inpt = header = index(header, ' ') + 1;

	while (*inpt && *inpt != ' ' && *inpt != '?') {
		inpt++;
	};
	end = inpt;
	oupt = ret = calloc(end - header + 1, sizeof(char));
	inpt = header;

	while (inpt != end) {
		if (*inpt == '%') {
			inpt++;
			high = *inpt++;
			low  = *inpt;
			*oupt = tohex(high, low);
		} else {
			*oupt = *inpt;
		}
		inpt++;
		oupt++;
	}
	*oupt = '\0';
	return ret;
}

int handle(int newsockfd, struct sockaddr_in socket, socklen_t socklen)
{
	char buffer[256], path[PATH_MAX], *url;
	struct stat path_stat;
	int n;

	(void) socklen;

	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0)
		error("error reading");

	url = geturl(buffer);

	info("%s GET %s", inet_ntoa(socket.sin_addr), url);

	snprintf(path, PATH_MAX, "%s/%s", basedir, url);

	while (n == 255)
		n = read(newsockfd, buffer, 255);

	if (stat(path, &path_stat)) {
		handle_notfound(newsockfd);
	} else {
		if (S_ISDIR(path_stat.st_mode)) {
			if (path[strlen(path)-1] != '/') {
				size_t len;
				len = strlen(url);
				url = realloc(url, len + 2);
				url[len] = '/';
				url[len+1] = 0;
				handle_redirection(url, url, newsockfd);
			} else {
				struct stat index_stat;
				char index_path[PATH_MAX];
				snprintf(index_path, PATH_MAX, "%s/index.html", path);
				index_stat.st_mode = 0;
				stat(index_path, &index_stat);
				if (S_ISREG(index_stat.st_mode))
					handle_file(url, index_path, newsockfd);
				else
					handle_directory(url, path, newsockfd);
			}
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

	(void) sig;

	wait(&a);
}

void abandonship(int sig)
{
	(void) sig;

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
	int one = 1;

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
		portno = 8081;
	else
		portno = 80;


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));

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

	info("Socket binded");

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
