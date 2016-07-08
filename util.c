#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include <time.h>

struct string STRING_EMPTY = {NULL, 0, 0};

char *stringf(struct string *s, const char *fmt, ...)
{
	va_list args;
	int size;

	if (!s)
		return NULL;

	va_start(args, fmt);
	size = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (size < 0)
		return NULL;

	if (s->content) {
		if (s->allocated < s->length + size + 1) {
			while (s->allocated < s->length + size + 1) {
				s->allocated *= 2;
			}
			s->content = realloc(s->content, s->allocated);
			if (!s->content)
				return NULL;
		}
	}
	else {
		s->allocated = size + 1;
		s->content = malloc(s->allocated);
		if (!s->content)
			return NULL;
		s->length = 0;
	}

	va_start(args, fmt);
	vsnprintf(s->content + s->length, size + 1, fmt, args);
	va_end(args);

	s->length += size;
	return s->content;
}

void error(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

void info(const char *format, ...)
{
	va_list args;

	time_t timer;
	struct tm * tmtime;
	char timebuffer[64];

	time(&timer);
	tmtime = localtime(&timer);
	strftime(timebuffer, 64, "%Y-%m-%d %H:%M:%S", tmtime);
	printf("%s ", timebuffer);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n');
}
