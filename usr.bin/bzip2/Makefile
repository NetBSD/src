#	$NetBSD: Makefile,v 1.5 2001/06/03 13:42:11 simonb Exp $

PROG=	   bzip2
LDSTATIC?= -static
LDDIR!=	   cd ${.CURDIR}/../../lib/libbz2 ; make print-objdir
LDADD+=	   -L${LDDIR} -lbz2
DPADD+=	   ${LIBBZ2}

DIST=	   ${.CURDIR}/../../dist/bzip2
.PATH:	   ${DIST}

.include <bsd.own.mk>

MLINKS+=   bzip2.1 bunzip2.1\
	   bzip2.1 bzcat.1\
	   bzip2.1 bzip2recover.1

LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bunzip2
LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bzcat

.include <bsd.prog.mk>
