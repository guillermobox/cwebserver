LDFLAGS += -lmagic
CFLAGS += -Wall -O2

.PHONY: clean

cwebserver: cwebserver.o handle_file.o handle_directory.o util.o

clean:
	rm -f *.o cwebserver
