CC=g++

# Falgs
CFLAGS=-I.
CFLAGS+=-Wall

# Source file
FILES=etherCtrl.cpp

# Target
TARGET=etherCtrl

# Default target
all: $(TARGET)

$(TARGET): $(FILES)
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

