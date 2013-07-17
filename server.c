#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <regex.h>

int sockfd = -1;

void error(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

#define BUFFLEN 256
void copy(char *filepath, int fdout)
{
	char *buffer, headers[128];
	struct stat st;
	int fd;
	time_t t;
	ssize_t nchars;

	fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		perror("opening file");
		exit(EXIT_FAILURE);
	}

	buffer = (char *) malloc(BUFFLEN * sizeof(char));

	stat(filepath, &st);

	t = time(NULL);

	bzero(headers, 128);
	strcpy(headers, "HTTP/1.1 200 OK\n");
	if (strncmp(filepath + strlen(filepath) - 5, ".html", 5) == 0)
		strcat(headers, "Content-Type: text/html\n");
	else
		strcat(headers, "Content-Type: text/plain\n");
	sprintf(headers + strlen(headers), "Content-length: %ld\n",
		st.st_size);
	strcat(headers, "Date: ");
	strcat(headers, ctime(&t));
	strcat(headers, "\n");

	write(fdout, headers, strlen(headers));

	while (1) {
		nchars = read(fd, buffer, BUFFLEN);
		if (nchars == 0) {
			break;
		} else if (nchars < 0) {
			perror("reading file");
			exit(1);
		} else {
			write(fdout, buffer, nchars);
		}
	}
	close(fd);
}

char *geturl(char *header)
{
	char *ptstart, *ptend;
	char *ret;

	ptstart = index(header, ' ') + 2;
	ptend = index(ptstart, ' ');

	ret = calloc(128, sizeof(char));
	strncpy(ret, ptstart, ptend - ptstart);
	ret[strlen(ret)] = '\0';
	return ret;
}

int handle(int newsockfd)
{
	char buffer[256], *path;
	int n;

	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0)
		error("error reading");

	path = geturl(buffer);
	if (strlen(path) == 0)
		puts(buffer);
	puts(path);

	while (n == 255)
		n = read(newsockfd, buffer, 255);

	if (access(path, R_OK) != 0)
		write(newsockfd, "HTTP/1.1 404 Not found\n", 23);
	else
		copy(path, newsockfd);

	close(newsockfd);
	free(path);
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
	int newsockfd, portno, pid;
	socklen_t clilen;
	char buffer[256];

	struct sockaddr_in serv_addr, cli_addr;
	int err;

	signal(SIGINT, &abandonship);
	signal(SIGCHLD, &waitforit);
	portno = 8080;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("error socketing");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	err =
	    bind(sockfd, (struct sockaddr *) &serv_addr,
		 sizeof(serv_addr));
	if (err < 0)
		error("error binding\n");

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1) {
		newsockfd =
		    accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			error("error accepting");

		switch (fork()) {
		case -1:
			error("error forking\n");
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
