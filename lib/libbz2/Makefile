#	$NetBSD: Makefile,v 1.20.8.1 2023/10/08 14:35:23 martin Exp $

USE_FORT?= yes	# data driven bugs?

NOMAN=		# defined

.include <bsd.own.mk>

LIB=		bz2
DIST=		${NETBSDSRCDIR}/external/bsd/bzip2/dist
.PATH:		${DIST}

SRCS=		blocksort.c huffman.c crctable.c randtable.c compress.c \
		decompress.c bzlib.c
INCS=		bzlib.h
INCSDIR=	/usr/include

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS+=		-Wno-error=implicit-fallthrough
.endif

# XXX blocksort.c gets mis-compiled with 4.1
.if (${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb") && \
    defined(HAVE_GCC)
COPTS.blocksort.c+=	-fno-loop-optimize
.endif

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/doc/reference/ref1/bzip2
FILES=		manual.html
.endif

.include <bsd.info.mk>
.include <bsd.lib.mk>
