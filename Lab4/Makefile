CC=g++

# Falgs
CFLAGS=-I.
CFLAGS+=-Wall

# Source file
FILES=Lab4.cpp

# Target
TARGET=Lab4

# Default target
all: $(TARGET)

$(TARGET): $(FILES)
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

