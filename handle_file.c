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
#include "mimetype.h"

static void handle_file_body(const char *path, const int fdout)
{
	const int BUFFLEN = 4096;
	char buffer[BUFFLEN];
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("opening file");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int nread, nwrite;

		nread = read(fd, buffer, BUFFLEN);
		if (nread == 0) {
			break;
		}
		else if (nread < 0) {
			perror("reading file");
			exit(1);
		}
		else {
			nwrite = 0;
			while (nread) {
				nwrite += write(fdout, buffer + nwrite, nread);
				nread -= nwrite;
			}
		}
	}
	close(fd);
}

static void handle_file_headers(const char *path, const int fdout)
{
	struct string headers = STRING_EMPTY;
	magic_t magic;
	time_t tnow;
	struct stat st;
	const char * mtype;

	magic = magic_open(MAGIC_MIME);
	if (magic == NULL) {
		error("No magic found!");
	}
	if (magic_load(magic, NULL)) {
		error("Impossible to load");
	}

	if (stat(path, &st) < 0) {
		perror("Getting file status");
		exit(EXIT_FAILURE);
	}

	tnow = time(NULL);

	mtype = guess_mimetype(path);
	if (mtype == NULL)
		mtype = magic_file(magic, path);

	stringf(&headers, "HTTP/1.1 200 OK\n");
	stringf(&headers, "Content-type: %s\n", mtype);
	stringf(&headers, "Content-length: %ld\n", st.st_size);
	stringf(&headers, "Date: %s\n", ctime(&tnow));

	write(fdout, headers.content, headers.length);

	magic_close(magic);
	free(headers.content);
}

void handle_file(const char *url, const char *path, int fdout)
{
	(void) url;

	handle_file_headers(path, fdout);
	handle_file_body(path, fdout);
}

