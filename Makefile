LDFLAGS += -lmagic
CFLAGS += -Wall -O2 -pedantic -Wextra -DHAS_MAGIC

.PHONY: clean

cwebserver: cwebserver.o handle_file.o handle_directory.o util.o mimetype.o

clean:
	rm -f *.o cwebserver
