# This is the Makefile for the libuucp subdirectory of Taylor UUCP
# $Id: Makefile,v 1.5 1997/05/07 16:20:29 mycroft Exp $

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CFLAGS+=	-I$(.CURDIR)/../common_sources

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include <bsd.lib.mk>
