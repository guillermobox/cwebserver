#include <string.h>

#if HAS_MAGIC
#include <magic.h>
#endif

struct st_mimetype {
	const char extension[8];
	const char mimetype[64];
};

static char default_mimetype[] = "text/plain";

static struct st_mimetype mimetypes[] = {
	{"au", "audio/basic"},
	{"avi", "video/avi"},
	{"bmp", "image/bmp"},
	{"bz2", "application/x-bzip2"},
	{"c", "text/x-c"},
	{"css", "text/css"},
	{"dtd", "application/xml-dtd"},
	{"doc", "application/msword"},
	{"exe", "application/octet-stream"},
	{"gif", "image/gif"},
	{"gz", "application/x-gzip"},
	{"hqx", "application/mac-binhex40"},
	{"html", "text/html"},
	{"ico", "image/x-icon"},
	{"jar", "application/java-archive"},
	{"jpg", "image/jpeg"},
	{"js", "application/x-javascript"},
	{"midi", "audio/x-midi"},
	{"mp3", "audio/mpeg"},
	{"mpeg", "video/mpeg"},
	{"ogg", "audio/vorbis"},
	{"pdf", "application/pdf"},
	{"pl", "application/x-perl"},
	{"png", "image/png"},
	{"ppt", "application/vnd.ms-powerpoint"},
	{"ps", "application/postscript"},
	{"qt", "video/quicktime"},
	{"rdf", "application/rdf"},
	{"rtf", "application/rtf"},
	{"sgml", "text/sgml"},
	{"sit", "application/x-stuffit"},
	{"svg", "image/svg+xml"},
	{"swf", "application/x-shockwave-flash"},
	{"ta","r.gz application/x-tar"},
	{"tgz", "application/x-tar"},
	{"tiff", "image/tiff"},
	{"tsv", "text/tab-separated-values"},
	{"txt", "text/plain"},
	{"wav", "audio/wav"},
	{"xls", "application/vnd.ms-excel"},
	{"xml", "application/xml"},
	{"zip", "application/zip"},
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

	if (ext != NULL) {
		len = sizeof(mimetypes) / sizeof(*mimetypes);
		for (i = 0; i < len; i++) {
			if (strcmp(ext, mimetypes[i].extension) == 0)
				return mimetypes[i].mimetype;
		}
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

