#	$NetBSD: Makefile,v 1.9.10.1 2008/03/26 18:52:45 bouyer Exp $

NOMAN=		# defined

.include <bsd.own.mk>

LIB=		bz2
DIST=		${NETBSDSRCDIR}/dist/bzip2
.PATH:		${DIST}

SRCS=		blocksort.c huffman.c crctable.c randtable.c compress.c \
		decompress.c bzlib.c
INCS=		bzlib.h
INCSDIR=	/usr/include

# XXX huffman.c gets mis-compiled with 2.95.3
.if ${MACHINE_ARCH} == "vax"
COPTS+=		-O0
.endif

.if ${MKSHARE} != "no"
FILESDIR=	${HTMLDOCDIR}/bzip2
FILES=		manual.html
.endif

.include <bsd.info.mk>
.include <bsd.lib.mk>
