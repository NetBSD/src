#	$NetBSD: Makefile,v 1.2 2001/10/19 03:10:16 tv Exp $

PROG=   prename
NOMAN=  t
CLEANFILES+= t1 t2

.PATH:  ${.CURDIR}/..

CPPFLAGS+= -D_POSIX_SOURCE

regress:
	@echo Testing posix rename with -D_POSIX_SOURCE
	./prename

.include <bsd.prog.mk>
