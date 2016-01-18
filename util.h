
void error(char *str);
void info(const char *format, ...);

/*
 * string manipulation routine (stringf)
 */
struct string {
	char *content;
	size_t allocated;
	size_t length;
};

struct string STRING_EMPTY = {NULL, 0, 0};

char *stringf(struct string *s, const char *fmt, ...);
