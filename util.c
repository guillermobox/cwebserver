#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void error(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

void info(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n');
}

