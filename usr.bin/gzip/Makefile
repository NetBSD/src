#	$NetBSD: Makefile,v 1.2 2003/12/23 06:23:32 mrg Exp $

PROG=		gzip

DPADD=		${LIBZ}
LDADD=		-lz

# XXX missing: zforce gzexe znew zmore zdiff zgrep
MLINKS+=	gzip.1 gunzip.1 \
		gzip.1 gzcat.1 \
		gzip.1 zcat.1 \
LINKS+=		${BINDIR}/gzip ${BINDIR}/gunzip \
		${BINDIR}/gzip ${BINDIR}/gzcat \
		${BINDIR}/gzip ${BINDIR}/zcat

.include <bsd.prog.mk>
