# This is the Makefile for the libuucp subdirectory of Taylor UUCP
# $Id: Makefile,v 1.6 1997/05/30 13:43:10 cjs Exp $

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CFLAGS+=	-I$(.CURDIR)/../common_sources

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# this library is used only for the build
NOINSTALL=1

.include <bsd.lib.mk>
