# $NetBSD: Makefile,v 1.1 1996/09/12 19:33:53 gwr Exp $
#

LIB=	z
SRCS=	adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
	zutil.c inflate.c infblock.c inftrees.c infcodes.c infutil.c inffast.c

CFLAGS+= -D_ZLIB_PRIVATE -I${.CURDIR}

NOMAN=

includes:
	-cd ${.CURDIR}; cmp -s zlib.h ${DESTDIR}/usr/include/zlib.h > \
	    /dev/null 2>&1 || \
	    install -c -o ${BINOWN} -g ${BINGRP} -m 444 zlib.h \
	    ${DESTDIR}/usr/include

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip : minigzip.c libz.a
	$(CC) -o minigzip minigzip.c libz.a
