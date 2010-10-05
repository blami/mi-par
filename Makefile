# Debugging
CFLAGS=-g
# Release
#CFLAGS=

srpgen: srpgen.c srputils.o srpfile.o
	gcc ${CFLAGS} -o srpgen srpgen.c srputils.o srpfile.o

srpfile.o: srpfile.c srpfile.h
	gcc ${CFLAGS} -c srpfile.c

srputils.o: srputils.c srputils.h
	gcc ${CFLAGS} -c srputils.c

clean:
	-rm -f *.o
	-rm srp-nompi srpgen
