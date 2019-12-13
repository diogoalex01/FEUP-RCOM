.PHONY: all clean
all: application

CFLAGS = -Wall -Wextra #-Werror
OBJECTS = application.o

application: $(OBJECTS)
	gcc $(CFLAGS) -pedantic $(OBJECTS) -o application

application.o = application.h

clean:
	rm -f application $(OBJECTS)