CC = gcc

CFLAGS = -Wall -Wextra -m64

TARGET = tracer

SRC = tracer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean