#	$NetBSD: Makefile,v 1.14 2004/05/23 02:24:50 lukem Exp $
#
# This is the Makefile for the libuucp subdirectory of Taylor UUCP

MKPRIVATELIB=	yes

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CPPFLAGS+=	-I$(.CURDIR)/../common_sources

.include <bsd.lib.mk>
