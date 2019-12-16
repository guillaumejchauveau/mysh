TARGET = mysh
LIBS = -lreadline
CC = gcc
CFLAGS = -Wall -Wextra

default: $(TARGET)
all: default

SRC = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, %.o, $(SRC))
HEADERS = $(wildcard src/*.h)

main.o: $(HEADERS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)