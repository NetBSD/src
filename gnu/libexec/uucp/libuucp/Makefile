# This is the Makefile for the libuucp subdirectory of Taylor UUCP

LIB=		uucp
SRCS=		buffer.o crc.o debug.o escape.o getopt.o getop1.o parse.o \
		spool.o status.o xfree.o xmall.o xreall.o getlin.o
CFLAGS+=	-I$(.CURDIR)/../common_sources

NOMAN=		noman
NOPROFILE=	noprofile

install:

.include <bsd.lib.mk>
