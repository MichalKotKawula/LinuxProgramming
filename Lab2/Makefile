CC=g++
CFLAGS=-I.
LDFLAGS=-L. -lPidUtil

all: Lab2

Lab2: Lab2.o libPidUtil.a
	$(CC) -o Lab2 Lab2.o $(LDFLAGS)

Lab2.o: Lab2.cpp pidutil.h
	$(CC) $(CFLAGS) -c Lab2.cpp
	
clean:
	rm -f *.o *.a Lab2
