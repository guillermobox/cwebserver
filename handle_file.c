#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <magic.h>

#include "util.h"

#define BUFFLEN 256
void handle_file(char *filepath, int fdout)
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

