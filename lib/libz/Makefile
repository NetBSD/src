#	$NetBSD: Makefile,v 1.15 2001/12/12 00:51:06 tv Exp $

LIB=		z
NOMAN=		# defined

SRCS=	adler32.c compress.c crc32.c deflate.c gzio.c infblock.c \
	infcodes.c inffast.c inflate.c inftrees.c infutil.c trees.c uncompr.c \
	zutil.c gzio_compat.c

CPPFLAGS+=	-I${.CURDIR}
CLEANFILES+=	minigzip

INCS=		zconf.h zlib.h
INCSDIR=	/usr/include

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip: minigzip.c libz.a
	${CC} -o minigzip ${.CURDIR}/minigzip.c libz.a
