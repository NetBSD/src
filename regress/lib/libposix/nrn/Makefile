#	$NetBSD: Makefile,v 1.2 2001/10/19 03:10:15 tv Exp $

PROG=   prename
NOMAN=  t
CLEANFILES+= t1 t2

.PATH:  ${.CURDIR}/..

CPPFLAGS+= -DBSD_RENAME

regress:
	@echo Testing BSD rename
	./prename

.include <bsd.prog.mk>
