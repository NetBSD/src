#       $NetBSD: Makefile,v 1.2 2000/07/24 00:00:21 mycroft Exp $

PROG=   skeytest
NOMAN=  t

.PATH:  ${.CURDIR}/..

LDADD+=	-lskey

regress: skeytest
	@echo Testing S/Key hash algorithms
	./skeytest

.include <bsd.prog.mk>

