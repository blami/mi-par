# Debugging
#CFLAGS=-g
# Release
CFLAGS=

all: srpgen srpnompi

srpnompi: srpnompi.c srputils.o srptask.o srpdump.o srphist.o srpstack.o
	gcc $(CFLAGS) -o srpnompi srpnompi.c srputils.c srptask.o srpdump.o srphist.o srpstack.o

srpgen: srpgen.c srputils.o srptask.o srpdump.o srphist.o
	gcc $(CFLAGS) -o srpgen srpgen.c srputils.o srptask.o srpdump.o srphist.o

srptask.o: srptask.c srptask.h
	gcc $(CFLAGS) -c srptask.c

srputils.o: srputils.c srputils.h
	gcc $(CFLAGS) -c srputils.c

srpdump.o: srpdump.c srpdump.h
	gcc $(CFLAGS) -c srpdump.c

srphist.o: srphist.c srphist.h
	gcc $(CFLAGS) -c srphist.c

srpstack.o: srpstack.c srpstack.h
	gcc $(CFLAGS) -c srpstack.c

clean:
	-rm -f *.o
	-rm srpgen srpnompi srpmpi
