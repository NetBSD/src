#	$NetBSD: Makefile,v 1.2.16.1 2002/01/10 19:40:31 thorpej Exp $

PROG	= loadbsd.ttp

OBJS	= loadbsd.o
HEADERS	= ../libtos/kparamb.h ../libtos/tosdefs.h

#
# Define COMPRESSED_READ when you want to load gzipped kernels
#
DEFS	= -DCOMPRESSED_READ
LDADD	= -lz

include ../Makefile.inc

${PROG}: ${OBJS} ${LDADD}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD} ${LIBS}
