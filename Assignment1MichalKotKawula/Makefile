CC=g++
CFLAGS=-I
CFLAGS+=-Wall
INTF=intfMonitor.cpp
NETWORK=networkMonitor.cpp

intfMonitor: $(INTF)
	$(CC) $(CFLAGS) $(INTF) -o intfMonitor

networkMonitor: $(NETWORK)
	$(CC) $(CFLAGS) $(NETWORK) -o networkMonitor

clean:
	rm -f *.o intfMonitor networkMonitor

all: intfMonitor networkMonitor
