#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
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
#include <time.h>
#include <arpa/inet.h>
#include <regex.h>
#include <magic.h>

int sockfd = -1;
char basedir[PATH_MAX];

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
	magic_t magic;

	magic = magic_open(MAGIC_MIME_TYPE);
	if (magic == NULL) {
		error("No magic found!");
	}
	if (magic_load(magic, NULL)) {
		error("Impossible to load");
	};

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
	sprintf(headers + strlen(headers),
		"Content-type: %s\n" "Content-length: %ld\n" "Date: %s\n",
		magic_file(magic, filepath), st.st_size, ctime(&t));

	write(fdout, headers, strlen(headers));

	while (1) {
		nchars = read(fd, buffer, BUFFLEN);
		if (nchars == 0) {
			break;
		}
		else if (nchars < 0) {
			perror("reading file");
			exit(1);
		}
		else {
			write(fdout, buffer, nchars);
		}
	}
	close(fd);
	magic_close(magic);
}

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
	int n;

	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0)
		error("error reading");

	url = geturl(buffer);
	snprintf(path, PATH_MAX, "%s/%s", basedir, url);

	while (n == 255)
		n = read(newsockfd, buffer, 255);

	if (access(path, R_OK) != 0)
		write(newsockfd, "HTTP/1.1 404 Not found\n", 23);
	else
		copy(path, newsockfd);

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

void info(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n');
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
