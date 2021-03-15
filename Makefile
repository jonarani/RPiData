PROGRAM = pi
SOURCES = $(shell ls *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
CFLAGS = -Wall -Wextra
LDFLAGS =
LDLIBS = -lpthread -lbcm2835
CC = gcc

.PHONY: all clean

all: $(PROGRAM)
$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	rm -f *.o

clean:
	rm -f $(PROGRAM) *.o