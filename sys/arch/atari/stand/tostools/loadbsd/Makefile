#	$NetBSD: Makefile,v 1.2 1999/06/23 19:26:13 leo Exp $

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
