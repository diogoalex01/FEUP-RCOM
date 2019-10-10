.PHONY: all clean
all: writenoncanonical

CFLAGS = -Wall -Wextra -Werror
OBJECTS = writenoncanonical.o ll.o

writenoncanonical: $(OBJECTS)
	gcc $(CFLAGS) -pedantic $(OBJECTS) -o writenoncanonical

writenoncanonical.o = ll.h
ll.o = ll.h

clean:
	rm -f writenoncanonical $(OBJECTS)