#	$NetBSD: Makefile,v 1.10 1999/02/13 02:54:24 lukem Exp $
#
# This is the Makefile for the libuucp subdirectory of Taylor UUCP

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CPPFLAGS+=	-I$(.CURDIR)/../common_sources

MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
