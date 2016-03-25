#include <string.h>

#if HAS_MAGIC
#include <magic.h>
#endif

struct st_mimetype {
	const char extension[8];
	const char mimetype[64];
};

static char default_mimetype[] = "application/binary";
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
	const char * ext;
	ext = get_file_extension(path);

	len = sizeof(mimetypes) / sizeof(*mimetypes);
	for (i = 0; i < len; i++) {
		if (strcmp(ext, mimetypes[i].extension) == 0)
			return mimetypes[i].mimetype;
	}

#if HAS_MAGIC
	magic_t magic;
	const char * mtype;
	magic = magic_open(MAGIC_MIME);
	if (magic == NULL) {
		return default_mimetype;
	}
	if (magic_load(magic, NULL)) {
		return default_mimetype;
	}
	mtype = magic_file(magic, path);
	magic_close(magic);
	return (mtype != NULL) ? mtype : default_mimetype;
#endif

	return default_mimetype;
}

