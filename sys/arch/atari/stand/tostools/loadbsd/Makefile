#	$NetBSD: Makefile,v 1.1.26.1 1999/07/01 23:03:49 thorpej Exp $

PROG	= loadbsd.ttp

OBJS	= loadbsd.o cread.o
HEADERS	= loader.h ../libtos/kparamb.h

#
# Define COMPRESSED_READ when you want to load gzipped kernels
#
DEFS	= -DCOMPRESSED_READ
LDADD	= -lz

include ../Makefile.inc

${PROG}: ${OBJS} ${LDADD}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
