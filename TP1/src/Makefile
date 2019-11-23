.PHONY: all clean
all: application

CFLAGS = -Wall -Wextra -Werror
OBJECTS = application.o ll.o

application: $(OBJECTS)
	gcc $(CFLAGS) -pedantic $(OBJECTS) -o application

application.o = application.h ll.h
ll.o = ll.h

clean:
	rm -f application $(OBJECTS)