#	$NetBSD: Makefile,v 1.1 1998/09/14 02:37:23 ross Exp $

.include <bsd.own.mk>

PROG=      bzip2
WARNS=	   0
CWARNFLAGS=-Wall -Wstrict-prototypes -Wpointer-arith
LDSTATIC?= -static

MLINKS+=   bunzip2 bzcat bzip2recover

LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bunzip2
LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bzcat

.include <bsd.prog.mk>
