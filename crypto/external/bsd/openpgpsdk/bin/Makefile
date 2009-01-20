# $NetBSD: Makefile,v 1.2 2009/01/20 16:58:09 agc Exp $

PROG=openpgp
LDADD+=-lopenpgpsdk -lcrypto -lz -lbz2
CPPFLAGS+=-I${.CURDIR}/../dist/include
WARNS=0
NOMAN= # defined

BINDIR=/usr/bin

.PATH: ${.CURDIR}/../dist/src/app

.include <bsd.prog.mk>
