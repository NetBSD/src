#	$NetBSD: Makefile,v 1.1 1999/02/21 00:20:11 mjl Exp $

PROG=   prn
NOMAN=  t
CLEANFILES+= t1 t2

.PATH:  ${.CURDIR}/..

CPPFLAGS+= -DBSD_RENAME

regress:
	@echo Testing BSD rename
	./prn

.include <bsd.prog.mk>
