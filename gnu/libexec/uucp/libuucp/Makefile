#	$NetBSD: Makefile,v 1.13 2002/09/19 03:09:31 lukem Exp $
#
# This is the Makefile for the libuucp subdirectory of Taylor UUCP

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CPPFLAGS+=	-I$(.CURDIR)/../common_sources

# only needed during build
libinstall::

.include <bsd.lib.mk>
