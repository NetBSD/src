#	$NetBSD: Makefile,v 1.4 2003/12/28 12:37:42 mrg Exp $

PROG=		gzip
MAN=		gzip.1 gzexe.1 zdiff.1 zforce.1 zmore.1 znew.1

DPADD=		${LIBZ}
LDADD=		-lz

# XXX missing: zgrep
SCRIPTS=	zforce gzexe znew zmore zdiff 

MLINKS+=	gzip.1 gunzip.1 \
		gzip.1 gzcat.1 \
		gzip.1 zcat.1

LINKS+=		${BINDIR}/gzip ${BINDIR}/gunzip \
		${BINDIR}/gzip ${BINDIR}/gzcat \
		${BINDIR}/gzip ${BINDIR}/zcat

.include <bsd.prog.mk>
