# a very simple makefile...
#	$Id: Makefile.boot,v 1.1 1994/03/05 00:34:30 cgd Exp $

CC=gcc
CFLAGS=-Wall -I. -g -O  -DMACHINE=\"sun4\" -Dnotyet

OBJ=arch.o buf.o compat.o cond.o dir.o hash.o job.o main.o make.o \
    parse.o str.o suff.o targ.o var.o util.o

pmake: ${OBJ}
	@echo 'make of make and make.0 started.'
	(cd lst.lib; make)
	${CC} *.o lst.lib/*.o -o pmake
#	nroff -h -man make.1 > make.0
#	@echo 'make of make and make.0 completed.'
