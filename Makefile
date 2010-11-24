CC=gcc
LIBS=
MPICC=mpicc
MPILIBS=

# Debugging
CFLAGS=-g
# Release
#CFLAGS=

all: srpgen srpnompi srpmpi

srpnompi: srpnompi.c srputils.c srptask.c srpdump.c srphist.c srpstack.c srpprint.h
	$(CC) $(CFLAGS) -o srpnompi \
		srpnompi.c \
		srputils.c \
		srptask.c \
		srpdump.c \
		srphist.c \
		srpstack.c \
		$(LIBS)

srpmpi: srpmpi.c srputils.c srptask.c srpdump.c srphist.c srpstack.c srpprint.h
	$(MPICC) $(CFLAGS) -o srpmpi -DMPI \
		srpmpi.c \
		srputils.c \
		srptask.c \
		srpdump.c \
		srphist.c \
		srpstack.c \
		$(MPILIBS)

srpgen: srpgen.c srputils.c srptask.c srpdump.c srphist.c srpprint.h
	$(CC) $(CFLAGS) -o srpgen \
		srpgen.c \
		srputils.c \
		srptask.c \
		srpdump.c \
		srphist.c

clean:
	-rm -f *.o
	-rm srpgen srpnompi srpmpi
