#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:11 tv Exp $

PROG=		prename
NOMAN=		# defined
CLEANFILES+=	t1 t2
DPADD+=		${LIBPOSIX}
LDADD+=		-lposix

.PATH:		${.CURDIR}/..

regress:
	@echo Testing posix rename with -lposix
	./prename

.include <bsd.prog.mk>
