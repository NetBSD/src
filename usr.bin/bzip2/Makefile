#	$NetBSD: Makefile,v 1.3 2000/07/03 02:51:15 matt Exp $

PROG=      bzip2
WARNS=	   0
CWARNFLAGS+=-Wall -Wstrict-prototypes -Wpointer-arith
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
