#	$NetBSD: Makefile,v 1.3 2001/10/10 14:24:48 leo Exp $

PROG	= loadbsd.ttp

OBJS	= loadbsd.o cread.o
HEADERS	= ../libtos/kparamb.h ../libtos/tosdefs.h

#
# Define COMPRESSED_READ when you want to load gzipped kernels
#
DEFS	= -DCOMPRESSED_READ
LDADD	= -lz

include ../Makefile.inc

${PROG}: ${OBJS} ${LDADD}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
