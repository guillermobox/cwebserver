#include <string.h>

struct st_mimetype {
	const char extension[8];
	const char mimetype[64];
};

static struct st_mimetype mimetypes[] = {
	{"css", "text/css"},
	{"c", "text/x-c"},
};

static const char * get_file_extension(const char * path) {
	const char * ext = strrchr(path, '.');
	if (!ext || ext == path) return NULL;
	return ext + 1;
}

const char * guess_mimetype(const char * path)
{
	size_t len, i;
	const char * ext = get_file_extension(path);

	len = sizeof(mimetypes) / sizeof(*mimetypes);
	for (i = 0; i < len; i++) {
		if (strcmp(ext, mimetypes[i].extension) == 0)
			return mimetypes[i].mimetype;
	}
	return NULL;
}

