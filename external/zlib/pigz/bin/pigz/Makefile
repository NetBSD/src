#	$NetBSD: Makefile,v 1.12 2023/06/03 09:09:09 lukem Exp $

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

MLINKS+=	pigz.1 unpigz.1
LINKS+=		${BINDIR}/pigz ${BINDIR}/unpigz

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
		zgrep.1 zfgrep.1 \
		zmore.1 zless.1

LINKS+=		${BINDIR}/pigz ${BINDIR}/gzip \
		${BINDIR}/pigz ${BINDIR}/gunzip \
		${BINDIR}/pigz ${BINDIR}/gzcat \
		${BINDIR}/pigz ${BINDIR}/zcat \
		${BINDIR}/zdiff ${BINDIR}/zcmp \
		${BINDIR}/zgrep ${BINDIR}/zegrep \
		${BINDIR}/zgrep ${BINDIR}/zfgrep \
		${BINDIR}/zmore ${BINDIR}/zless
.endif

.include <bsd.prog.mk>

DIST=		${NETBSDSRCDIR}/external/zlib/pigz/dist
.PATH:	${DIST} ${DIST}/zopfli

.if ${MKPIGZGZIP} != "no"
GZIPDIST=	${NETBSDSRCDIR}/usr.bin/gzip
.PATH:	${GZIPDIST}
.endif

.if !defined(HOSTPROG)
COPTS.pigz.c+=	${CC_WNO_STRINGOP_OVERFLOW}
.endif
