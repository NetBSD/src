#	$NetBSD: Makefile,v 1.6 2001/06/10 00:17:51 enami Exp $

.include <bsd.own.mk>

PROG=	   bzip2
LDSTATIC?= -static
LDDIR!=	   cd ${.CURDIR}/../../lib/libbz2 && ${PRINTOBJDIR}
LDADD+=	   -L${LDDIR} -lbz2
DPADD+=	   ${LIBBZ2}

DIST=	   ${.CURDIR}/../../dist/bzip2
.PATH:	   ${DIST}

MLINKS+=   bzip2.1 bunzip2.1\
	   bzip2.1 bzcat.1\
	   bzip2.1 bzip2recover.1

LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bunzip2
LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bzcat

.include <bsd.prog.mk>
