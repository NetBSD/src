#       $NetBSD: Makefile,v 1.1 2000/07/06 22:35:58 mjl Exp $

PROG=   skeytest
NOMAN=  t

.PATH:  ${.CURDIR}/..

LDFLAGS += -lskey

regress: skeytest
	@echo Testing S/Key hash algorithms
	./skeytest

.include <bsd.prog.mk>

