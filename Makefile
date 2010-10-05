# Debugging
CFLAGS=-g
# Release
#CFLAGS=

srpgen: srpgen.c srputils.o srptask.o srpdump.o
	gcc $(CFLAGS) -o srpgen srpgen.c srputils.o srptask.o srpdump.o

srptask.o: srptask.c srptask.h
	gcc $(CFLAGS) -c srptask.c

srputils.o: srputils.c srputils.h
	gcc $(CFLAGS) -c srputils.c

srpdump.o: srpdump.c srpdump.h
	gcc $(CFLAGS) -c srpdump.c

clean:
	-rm -f *.o
	-rm srpgen
