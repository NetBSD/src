#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:12 tv Exp $

PROG=		prename
NOMAN=		# defined
CPPFLAGS+=	-D_POSIX_SOURCE
CLEANFILES+=	t1 t2

.PATH:		${.CURDIR}/..

regress:
	@echo Testing posix rename with -D_POSIX_SOURCE
	./prename

.include <bsd.prog.mk>
