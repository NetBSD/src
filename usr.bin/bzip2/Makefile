#	$NetBSD: Makefile,v 1.4 2001/02/05 01:35:45 christos Exp $

PROG=      bzip2
LDSTATIC?= -static
LDADD+=	   -lbz2
DPADD+=	   ${LIBBZ2}

.include <bsd.own.mk>

MLINKS+=   bzip2.1 bunzip2.1\
	   bzip2.1 bzcat.1\
	   bzip2.1 bzip2recover.1

LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bunzip2
LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bzcat

.include <bsd.prog.mk>
