#	$NetBSD: Makefile,v 1.4 1996/10/18 05:45:06 thorpej Exp $

LIB=	z
SRCS=	adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
	zutil.c inflate.c infblock.c inftrees.c infcodes.c infutil.c inffast.c

CFLAGS+= -D_ZLIB_PRIVATE -I${.CURDIR}

NOMAN=

CLEANFILES+= minigzip

includes:
	-cd ${.CURDIR}; cmp -s zlib.h ${DESTDIR}/usr/include/zlib.h > \
	    /dev/null 2>&1 || \
	    ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 zlib.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip : minigzip.c libz.a
	$(CC) -o minigzip ${.CURDIR}/minigzip.c libz.a
