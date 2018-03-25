all: buf

buf: buf.o
	g++ buf.o -o buff

buf.o: buf.cpp
	g++ -c buf.cpp

debug:
	g++ -ggdb3 buffer.h buf.cpp -o buff