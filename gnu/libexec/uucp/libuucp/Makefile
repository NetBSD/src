#	$NetBSD: Makefile,v 1.12 2001/12/12 01:49:00 tv Exp $
#
# This is the Makefile for the libuucp subdirectory of Taylor UUCP

LIB=		uucp
SRCS=		buffer.c crc.c debug.c escape.c getopt.c getop1.c parse.c \
		spool.c status.c xfree.c xmall.c xreall.c getlin.c
CPPFLAGS+=	-I$(.CURDIR)/../common_sources

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
