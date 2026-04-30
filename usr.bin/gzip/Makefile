#	$NetBSD: Makefile,v 1.21 2026/04/30 20:49:55 christos Exp $

.include <bsd.init.mk>
.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"

USE_FORT?= yes	# data-driven bugs?

PROG=		gzip
SRCS+=		gzip.c
MAN=		gzip.1 gzexe.1 zdiff.1 zforce.1 zgrep.1 zmore.1 znew.1

.if !defined(HOSTPROG)

DPADD=		${LIBZ} ${LIBBZ2} ${LIBLZMA}
LDADD=		-lz -lbz2 -llzma

.if ${CRUNCHEDPROG:U0} == 1
CPPFLAGS+=-DNO_ZSTD_SUPPORT
.else
CPPFLAGS+=	${ZSTDCPPFLAGS}
PROGDPLIBS+=	${ZSTDDPLIBS}
.endif

SCRIPTS=	gzexe zdiff zforce zgrep zmore znew

MLINKS+=	gzip.1 gunzip.1 \
		gzip.1 gzcat.1 \
		gzip.1 zcat.1 \
		zdiff.1 zcmp.1 \
		zgrep.1 zegrep.1 \
		zgrep.1 zfgrep.1 \
		zmore.1 zless.1

LINKS+=		${BINDIR}/gzip ${BINDIR}/gunzip \
		${BINDIR}/gzip ${BINDIR}/gzcat \
		${BINDIR}/gzip ${BINDIR}/zcat \
		${BINDIR}/zdiff ${BINDIR}/zcmp \
		${BINDIR}/zgrep ${BINDIR}/zegrep \
		${BINDIR}/zgrep ${BINDIR}/zfgrep \
		${BINDIR}/zmore ${BINDIR}/zless
.endif

.include <bsd.prog.mk>
