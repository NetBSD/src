#	$NetBSD: Makefile,v 1.8 1997/10/09 12:06:58 lukem Exp $

LIB=	z
NOMAN=	zlib.3

SRCS=	adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
	zutil.c inflate.c infblock.c inftrees.c infcodes.c infutil.c inffast.c
WARNS?=	1

CFLAGS+= -I${.CURDIR}

CLEANFILES+= minigzip

INCS=	zconf.h zlib.h
INCSDIR=/usr/include

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip : minigzip.c libz.a
	$(CC) -o minigzip ${.CURDIR}/minigzip.c libz.a
