#	$NetBSD: Makefile,v 1.19 2006/01/14 20:12:36 christos Exp $

.include <bsd.own.mk>

USE_SHLIBDIR=	yes

ZLIBDIR=${NETBSDSRCDIR}/common/dist/zlib
.PATH: ${ZLIBDIR}
LIB=		z

SRCS=	adler32.c compress.c crc32.c deflate.c gzio.c infback.c inffast.c \
	inflate.c inftrees.c trees.c uncompr.c zutil.c 
MAN=	zlib.3

CPPFLAGS+=	-I${ZLIBDIR}
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
