#       $NetBSD: Makefile,v 1.3 2000/07/27 00:25:05 cgd Exp $

PROG=   skeytest
NOMAN=  t

.PATH:  ${.CURDIR}/..

DPADD+=	${LIBSKEY}
LDADD+=	-lskey

regress: skeytest
	@echo Testing S/Key hash algorithms
	./skeytest

.include <bsd.prog.mk>

