# a very simple makefile...
#	$Id: Makefile.boot,v 1.2 1994/04/09 23:35:25 briggs Exp $

CC=gcc
CFLAGS=-Wall -I. -g -O  -DMACHINE=\"sun4\" -Dnotyet

OBJ=arch.o buf.o compat.o cond.o dir.o for.o hash.o job.o main.o make.o \
    parse.o str.o suff.o targ.o var.o util.o

pmake: ${OBJ}
	@echo 'make of make and make.0 started.'
	(cd lst.lib; make)
	${CC} *.o lst.lib/*.o -o pmake
#	nroff -h -man make.1 > make.0
#	@echo 'make of make and make.0 completed.'
