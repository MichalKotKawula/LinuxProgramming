CC=g++
CFLAGS=-I
CFLAGS+=-Wall
FILES=Logger.cpp Automobile.cpp TravelSimulator.cpp
FILES2=LogServer.cpp
LIBS=-lpthread

travelSimulator: $(FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

logServer: $(FILES2)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f *.o travelSimulator logServer
	
all: travelSimulator logServer
