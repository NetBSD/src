#       $NetBSD: Makefile,v 1.5 2002/09/18 05:41:49 lukem Exp $

NOMAN=		# defined

PROG=		skeytest
DPADD+=		${LIBSKEY}
LDADD+=		-lskey

.PATH:		${.CURDIR}/..

regress: skeytest
	@echo Testing S/Key hash algorithms
	./skeytest

.include <bsd.prog.mk>

