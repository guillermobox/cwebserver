LDFLAGS +=
CFLAGS += -Wall -O2 -pedantic -Wextra

Q := @

.PHONY: clean

cwebserver: cwebserver.o handle_file.o handle_directory.o util.o mimetype.o
	@echo "  LD  " $@
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	@echo "  CC  " $@
	$(Q)$(CC) -c $(CFLAGS) $^ -o $@

clean:
	@echo " CLEAN "
	$(Q)rm -f *.o cwebserver
