
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

extern struct string STRING_EMPTY;

char *stringf(struct string *s, const char *fmt, ...);
