#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include "util.h"
#include "assets.h"

extern char * basedir;

static int sort_dirs(const void *a, const void *b)
{
	struct dirent *da = (struct dirent *) a;
	struct dirent *db = (struct dirent *) b;
	
	if (da->d_type != db->d_type) {
		return da->d_type - db->d_type;
	} else {
		return strcmp(da->d_name, db->d_name);
	}
}

static char * filesize(struct stat st)
{
	static char fsstring[32];
	float size = (float) st.st_size;
	char *mul = "\0kMGT";

	while (size > 1024) {
		size /= 1024;
		mul++;
	}
	if (*mul) {
		snprintf(fsstring, 32, "%.1f %cB", size, *mul);
	} else {
		snprintf(fsstring, 32, "%.0f B", size);
	}
	return fsstring;
}

static char * format_direntry(const char *url, const char * dirpath, struct dirent * entry)
{
	static char buffer[1024];
	char path[PATH_MAX];
	struct stat st;
	
	snprintf(path, PATH_MAX, "%s/%s", dirpath, entry->d_name);
	stat(path, &st);

	if (S_ISDIR(st.st_mode)) {
		snprintf(buffer, 1024, "<li class=\"folder\"><a href=\"%s%s\">%s</a></li>\n", strlen(url) > 1? url : "", entry->d_name, entry->d_name);
	} else {
		snprintf(buffer, 1024, "<li class=\"file\"><a href=\"%s%s\">%s</a> [%s]</li>\n", strlen(url) > 1? url : "", entry->d_name, entry->d_name, filesize(st));
	}

	return buffer;
}

void handle_directory(const char * url, const char * path, int fdout)
{
	char headers[128];
	DIR * dir;
	struct dirent * entry, * entries;
	size_t allocated, used;
	time_t t;
	struct string buffer = STRING_EMPTY;

	bzero(headers, 128);

	dir = opendir(path);
	if (dir == NULL) {
		perror("opening dir");
		exit(EXIT_FAILURE);
	}

	stringf(&buffer, "<!DOCTYPE html>\n"
			"<html>\n"
			"<meta charset=\"UTF-8\">\n"
			"<head>\n<style type=\"text/css\">li.folder{\nfont-family:monospace;list-style-image: url(data:image/png;base64,%s);list-style-position:inside;\n}\nli.file{\nfont-family:monospace;list-style-image: url(data:image/png;base64,%s);list-style-position: inside;\n}\n</style>\n<title>%s</title></head>\n"
			"<body><p><b>Directory contents of %s</b></p><ul>",
			folder_png, file_png, url, url);
	allocated = 16;
	used = 0;
	entries = malloc(sizeof(struct dirent) * allocated);

	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		if (used == allocated) {
			allocated *= 2;
			entries = realloc(entries, sizeof(struct dirent) * allocated);
		}
		memcpy(entries + used, entry, sizeof(struct dirent));
		used++;
	}

	qsort(entries, used, sizeof(struct dirent), sort_dirs);

	entry = entries;
	while (used) {
		stringf(&buffer, "%s", format_direntry(url, path, entry));
		used--;
		entry++;
	}

	stringf(&buffer, "</ul></body>\n</html>\n");

	t = time(NULL);

	strcpy(headers, "HTTP/1.1 200 OK\n");
	sprintf(headers + strlen(headers),
		"Content-type: %s\n" "Content-length: %lu\n" "Date: %s\n",
		"text/html", (unsigned long) buffer.length, ctime(&t));

	if (write(fdout, headers, strlen(headers)) != (ssize_t) strlen(headers)) {
		error("error writing directory headers");
	};
	if (write(fdout, buffer.content, buffer.length) != (ssize_t) buffer.length) {
		error("error writing directory content");
	};

	free(entries);
	closedir(dir);
}

