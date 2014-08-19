#	$NetBSD: Makefile,v 1.7.10.1 2014/08/20 00:00:21 tls Exp $

.include <bsd.own.mk>

BINDIR=		/usr/bin

USE_FORT?= yes	# data-driven bugs?

CWARNFLAGS.clang+=	-Wno-format

PROG=		pigz
SRCS=		pigz.c yarn.c
MAN=		pigz.1

# pigz 2.3.1 brings zopfli support
SRCS+=		deflate.c blocksplitter.c tree.c lz77.c cache.c hash.c util.c \
		squeeze.c katajainen.c

.if !defined(HOSTPROG)
DPADD=		${LIBM} ${LIBPTHREAD}
.endif
LDADD=		-lm -lz -lpthread

.if ${MKPIGZGZIP} != "no"
# these ones are from mrg's gzip
MAN+=		gzexe.1 zdiff.1 zforce.1 zgrep.1 zmore.1 znew.1

SCRIPTS=	gzexe zdiff zforce zgrep zmore znew

MLINKS+=	pigz.1 gzip.1 \
		pigz.1 gunzip.1 \
		pigz.1 gzcat.1 \
		pigz.1 zcat.1 \
		zdiff.1 zcmp.1 \
		zgrep.1 zegrep.1 \
		zgrep.1 zfgrep.1

LINKS+=		${BINDIR}/pigz ${BINDIR}/gzip \
		${BINDIR}/pigz ${BINDIR}/gunzip \
		${BINDIR}/pigz ${BINDIR}/gzcat \
		${BINDIR}/pigz ${BINDIR}/zcat \
		${BINDIR}/zdiff ${BINDIR}/zcmp \
		${BINDIR}/zgrep ${BINDIR}/zegrep \
		${BINDIR}/zgrep ${BINDIR}/zfgrep
.endif

.include <bsd.prog.mk>

DIST=		${NETBSDSRCDIR}/external/zlib/pigz/dist
.PATH:	${DIST} ${DIST}/zopfli

.if ${MKPIGZGZIP} != "no"
GZIPDIST=	${NETBSDSRCDIR}/usr.bin/gzip
.PATH:	${GZIPDIST}
.endif
