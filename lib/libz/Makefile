#	$NetBSD: Makefile,v 1.6 1997/05/13 23:01:08 gwr Exp $

LIB=	z
NOMAN=	zlib.3

HDRS=	zconf.h zlib.h
SRCS=	adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
	zutil.c inflate.c infblock.c inftrees.c infcodes.c infutil.c inffast.c

CFLAGS+= -I${.CURDIR}

CLEANFILES+= minigzip

INCS=zlib.h
INCSDIR=/usr/include

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip : minigzip.c libz.a
	$(CC) -o minigzip ${.CURDIR}/minigzip.c libz.a
