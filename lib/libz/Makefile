#	$NetBSD: Makefile,v 1.20.34.1 2012/04/17 00:05:35 yamt Exp $

WARNS=	3

USE_FORT?= yes	# data-driven bugs?

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
