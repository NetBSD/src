#	$NetBSD: Makefile,v 1.2 1998/01/05 20:51:30 perry Exp $
#
CC = gcc
CFLAGS = -m68030 -O2 -D__progname=program_name -noixemul

OBJS = loadbsd.o getopt.o
LIBS = -lamiga

loadbsd: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o loadbsd $(OBJS) $(LIBS)

clean:
	delete $(OBJS) loadbsd
