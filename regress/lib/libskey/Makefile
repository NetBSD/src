#       $NetBSD: Makefile,v 1.4 2001/12/12 01:24:12 tv Exp $

PROG=		skeytest
NOMAN=		# defined
DPADD+=		${LIBSKEY}
LDADD+=		-lskey

.PATH:		${.CURDIR}/..

regress: skeytest
	@echo Testing S/Key hash algorithms
	./skeytest

.include <bsd.prog.mk>

