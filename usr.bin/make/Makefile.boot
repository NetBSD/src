#	$NetBSD: Makefile.boot,v 1.24 2021/12/12 11:02:23 rillig Exp $
#
# A very simple makefile...
#
# You only want to use this if you aren't running NetBSD.
#
# Modify MACHINE and MACHINE_ARCH as appropriate for your target architecture.
# See config.h and the various #ifdef directives for further configuration, in
# particular in util.c.
#

PROG=		bmake
MACHINE=	i386
MACHINE_ARCH=	i386
CC=		gcc
CFLAGS=		-O -g
EXTRA_CFLAGS=
EXTRA_LIBS=

OBJS=	arch.o buf.o compat.o cond.o dir.o for.o hash.o \
	job.o lst.o main.o make.o make_malloc.o metachar.o parse.o \
	str.o suff.o targ.o trace.o var.o util.o

.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} ${EXTRA_CFLAGS} -c $< -o $@

CPPFLAGS= \
	-DTARGET_MACHINE=\"${MACHINE}\" \
	-DTARGET_MACHINE_ARCH=\"${MACHINE_ARCH}\" \
	-DMAKE_MACHINE=\"${MACHINE}\"

${PROG}: ${OBJS}
#	@echo 'make of ${PROG} and make.0 started.'
	${CC} ${CFLAGS} ${OBJS} -o $@ ${EXTRA_LIBS}
	@ls -l $@
#	nroff -h -man make.1 > make.0
#	@echo 'make of ${PROG} and make.0 completed.'

clean:
	rm -f ${OBJS} ${PROG}
