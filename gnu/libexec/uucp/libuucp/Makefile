#	$NetBSD: Makefile,v 1.9 1997/10/25 09:31:39 lukem Exp $
#
# This is the Makefile for the libuucp subdirectory of Taylor UUCP

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CPPFLAGS+=	-I$(.CURDIR)/../common_sources

NOMAN=		noman
NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include <bsd.lib.mk>
