#	$NetBSD: Makefile,v 1.5 2003/12/28 13:37:23 mrg Exp $

PROG=		gzip
MAN=		gzip.1 gzexe.1 zdiff.1 zforce.1 zmore.1 znew.1

DPADD=		${LIBZ}
LDADD=		-lz

# XXX missing: zgrep
SCRIPTS=	zforce gzexe znew zmore zdiff 

# XXX missing: zgrep.1->zegrep.1 zgrep.1->zfgrep.1
MLINKS+=	gzip.1 gunzip.1 \
		gzip.1 gzcat.1 \
		gzip.1 zcat.1 \
		zdiff.1 zcmp.1

# XXX missing: zgrep->zegrep zgrep->zfgrep
LINKS+=		${BINDIR}/gzip ${BINDIR}/gunzip \
		${BINDIR}/gzip ${BINDIR}/gzcat \
		${BINDIR}/gzip ${BINDIR}/zcat
		${BINDIR}/zdiff ${BINDIR}/zcmp

.include <bsd.prog.mk>
