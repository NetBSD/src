#	$NetBSD: Makefile,v 1.1 1999/02/21 00:20:11 mjl Exp $

PROG=   prn
NOMAN=  t
CLEANFILES+= t1 t2

.PATH:  ${.CURDIR}/..

CPPFLAGS+= -D_POSIX_SOURCE

regress:
	@echo Testing posix rename with -D_POSIX_SOURCE
	./prn

.include <bsd.prog.mk>
