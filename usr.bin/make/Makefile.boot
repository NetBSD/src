# a very simple makefile...
#	$Id: Makefile.boot,v 1.3 1994/04/17 03:09:49 glass Exp $
#
# You only want to use this if you aren't running NetBSD.
#
# modify MACHINE and MACHINE_ARCH as appropriate for your target architecture
#
CC=gcc
CFLAGS=-Wall -I. -g -O  -DMACHINE=\"sparc\" -DMACHINE_ARCH=\"sparc\" \
	-DMAKE_BOOTSTRAP

OBJ=arch.o buf.o compat.o cond.o dir.o for.o hash.o job.o main.o make.o \
    parse.o str.o suff.o targ.o var.o util.o

pmake: ${OBJ}
	@echo 'make of make and make.0 started.'
	(cd lst.lib; make)
	${CC} *.o lst.lib/*.o -o pmake
#	nroff -h -man make.1 > make.0
#	@echo 'make of make and make.0 completed.'
