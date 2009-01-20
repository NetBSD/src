# $NetBSD: Makefile,v 1.1 2009/01/20 07:50:54 agc Exp $

PROG=openpgp
LDADD+=-lopenpgpsdk -lcrypto -lz -lbz2
CPPFLAGS+=-I${.CURDIR}/../dist/include
WARNS=0
MKMAN=no

.PATH: ${.CURDIR}/../dist/src/app

.include <bsd.prog.mk>
