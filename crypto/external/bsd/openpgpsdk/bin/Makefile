# $NetBSD: Makefile,v 1.3 2009/01/20 19:48:23 agc Exp $

PROG=openpgp
LDADD+=-lopenpgpsdk
CPPFLAGS+=-I${.CURDIR}/../dist/include
WARNS=0
NOMAN= # defined

BINDIR=/usr/bin

.PATH: ${.CURDIR}/../dist/src/app

.include <bsd.prog.mk>
