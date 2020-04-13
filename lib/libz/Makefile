#	$NetBSD: Makefile,v 1.23.14.2 2020/04/13 08:03:17 martin Exp $

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

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.infback.c+=	-Wno-error=implicit-fallthrough
COPTS.inflate.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.lib.mk>

test: minigzip
	echo hello world | ./minigzip | ./minigzip -d

# Note: CFLAGS ommitted by intention!
# This is to verify that zlib.h works standalone.
minigzip: ${ZLIBDIR}/test/minigzip.c libz.a
	${CC} -o minigzip ${ZLIBDIR}/test/minigzip.c libz.a
