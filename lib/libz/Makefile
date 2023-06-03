#	$NetBSD: Makefile,v 1.28 2023/06/03 09:09:11 lukem Exp $

WARNS=	3

USE_FORT?= yes	# data-driven bugs?

.include <bsd.own.mk>

USE_SHLIBDIR=	yes

ZLIBDIR=${NETBSDSRCDIR}/common/dist/zlib
.PATH: ${ZLIBDIR}
LIB=		z

SRCS=	adler32.c compress.c crc32.c deflate.c infback.c inffast.c \
	inflate.c inftrees.c trees.c uncompr.c zutil.c gzwrite.c \
	gzlib.c gzread.c gzclose.c
MAN=	zlib.3

CPPFLAGS+=	-I${ZLIBDIR}
CLEANFILES+=	minigzip

INCS=		zconf.h zlib.h
INCSDIR=	/usr/include

FILES+=			zlib.pc
FILESOWN_zlib.pc=	${BINOWN}
FILESGRP_zlib.pc=	${BINGRP}
FILESMODE_zlib.pc=	${NONBINMODE}
FILESDIR_zlib.pc=	/usr/lib/pkgconfig

COPTS.infback.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.inflate.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS omitted by intention!
# This is to verify that zlib.h works standalone.
minigzip: ${ZLIBDIR}/test/minigzip.c libz.a
	${CC} -o minigzip ${ZLIBDIR}/test/minigzip.c libz.a
