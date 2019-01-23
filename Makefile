TARGET = microhttpd
LIBS = -lmicrohttpd
CC = gcc
CFLAGS = -g -Wall 

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

.PHONY: clean all default

default: $(TARGET)
all: default

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o *.bak *.c~ *.out
	-rm -f $(TARGET)

indent: %.c
	indent -gnu $@
