#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

extern char * basedir;

static char * format_direntry(const char * dirpath, struct dirent * entry)
{
	static char buffer[256];
	char path[PATH_MAX];
	struct stat st;
	
	snprintf(path, PATH_MAX, "%s/%s", dirpath, entry->d_name);
	stat(path, &st);

	if (S_ISDIR(st.st_mode)) {
		snprintf(buffer, 256, "<li class=\"folder\"><a href=\"%s\">%s</a></li>\n", entry->d_name, entry->d_name);
	} else {
		snprintf(buffer, 256, "<li class=\"file\"><a href=\"%s\">%s</a></li>\n", entry->d_name, entry->d_name);
	}

	return buffer;
}

void handle_directory(const char * url, const char * path, int fdout)
{
	char headers[128], buffer[1024 * 256];
	DIR * dir;
	struct dirent * entry;
	time_t t;

	bzero(buffer, 1024 * 256);
	bzero(headers, 128);

	dir = opendir(path);
	if (dir == NULL) {
		perror("opening dir");
		exit(EXIT_FAILURE);
	}

	sprintf(buffer, "<!DOCTYPE html>\n"
			"<html>\n"
			"<head>\n<style type=\"text/css\">li.folder{\nlist-style-image: url(data:image/png;base64,%s);list-style-position:inside;\n}\nli.file{\nlist-style-image: url(data:image/png;base64,%s);list-style-position: inside;\n}\n</style>\n<title>%s</title></head>\n"
			"<body><h1>Directory contents</h1><ul>",
			"iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAN1wAADdcBQiibeAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAAOdEVYdFRpdGxlAEZvbGRlcnMhVKpgQQAAABd0RVh0QXV0aG9yAExhcG8gQ2FsYW1hbmRyZWnfkRoqAAAAKXRFWHREZXNjcmlwdGlvbgBCYXNlZCBvZiBKYWt1YiBTdGVpbmVyIGRlc2lnbqCEBXMAAAGHSURBVDiNpZM/a5NRFId/53rTRGMaE4NODuLgUKEdbSkInepgKXWyswiK4ufwAyh+iFJ0cpEWdJAu/mk7FFpBcImF0peY5E3uue/5OTSiAfNC4x3ucM89z3nOgSMk8T9HHt+98RTkM0CKwxEcW+SD569213IBT1amWkurjyqFQgE0g1kGswyddgubb9a6oZfOvni9t51jMKXL9x767wdf/ipOTHjCoubq/2y3N7yDIO22/rzS4BAxPbuE+qUrI5NjSPF2/eWCr9dr8uPbLrJBtYnSOUzP3UalWkPS3B8JqF6+hqARvtG4eGZ+cRXVWgPkiUG/m6CTNHP1LVOoKryqojJ5AUnza27CKIALQWFZ/rD+DYgIQeFUFZbFMQADA9U4toFqHBjE8Q18UAUtg4gb/iG/r8GuDK0MQYsIqvCkWSftunL5PEQcROQEJg7iBCBBEqSBJEADaUiODkGaeSPeffywcevm/IKcpoWdT1sk8V7u37l+tVw6u97v92ZOAygWS587vXTlF5yo2ogR3/0GAAAAAElFTkSuQmCC",
			"iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAN1wAADdcBQiibeAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAASdEVYdFRpdGxlAFBhcGVyIFNoZWV0c7mvkfkAAAAXdEVYdEF1dGhvcgBMYXBvIENhbGFtYW5kcmVp35EaKgAAACd0RVh0RGVzY3JpcHRpb24Ad2l0aCBhIEhVR0UgaGVscCBmcm9tIEpha3VihlQHswAAAhNJREFUOI11kstqU1EUhr91ctI2A2uTNsRaOxDEkeILiIgTL6CCAx+iUnTSgQPBRxAFSxWhA8XiBQst7aQjUV+kMWlzOaeJVZvsy3JwctK0wQWLvQabb/3/v7eoKuubqzdFZMk5PwuKqqIKoAB/Qba8d8/v3b2/xfFSVVbXPpWbUUO990Pd7Xa0Uv2paxurf1Y+vnucwA87AOh0OjP5iQL7v/dptWOacZ1ao0plZ5vdepV2q8Wt67dzxanik7fvlxcGBQQAxlgAqpUK5e0KO5Ua9d2IuNlmL/pFuVwhCAKuXrmWGx0Ze/pm+dXlFBAmAANAYSqPcy5p73DO4pwjE8OHzyuMZXNcvHAp9/3H1wXgWx9gjQGURi3CWjuU01S+xMkTBbxYgiCQg4ODGy9ePsvMzz1yfQUKTBTGcc7iVVHv8T5V4hhhFJExzp09z8bmesarzwIpINkaN1s454YUpCWBkC706gcysEkG+clxnPNo7y/0PsMhQHoAa1CvwyFCQBAoipBcFY4eyWCtxTt/FCBAHO3h7P8tZMIMpeI0xlh8z+pABkLpVBG0J1UGVKQKVBARrDH9rAaeERq1iG63298YhiFnZmf63rWXiTEGd9wCwOmZaUTkaA8ooJfpEEBEqnEcTRcKk//1n1a73QIkMtZ0EluqzD98cCfMhoum2y2pgpI84fEZlGx2pG6MmVtafP0F4B+wR1eZMTEGTgAAAABJRU5ErkJggg==",
			path);

	while ((entry = readdir(dir))) {
		sprintf(buffer + strlen(buffer), format_direntry(path, entry));
	}

	strcat(buffer,"</ul></body>\n</html>\n");

	t = time(NULL);

	strcpy(headers, "HTTP/1.1 200 OK\n");
	sprintf(headers + strlen(headers),
		"Content-type: %s\n" "Content-length: %ld\n" "Date: %s\n",
		"text/html", strlen(buffer), ctime(&t));

	write(fdout, headers, strlen(headers));
	write(fdout, buffer, strlen(buffer));

	closedir(dir);
}

