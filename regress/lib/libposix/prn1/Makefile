#	$NetBSD: Makefile,v 1.1 1999/02/21 00:20:11 mjl Exp $

PROG=   prn
NOMAN=  t
CLEANFILES+= t1 t2

.PATH:  ${.CURDIR}/..

DPADD+=	${LIBPOSIX}
LDADD+=	-lposix

regress:
	@echo Testing posix rename with -lposix
	./prn

.include <bsd.prog.mk>
