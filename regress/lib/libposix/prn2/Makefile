#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:49 lukem Exp $

NOMAN=		# defined

PROG=		prename
CPPFLAGS+=	-D_POSIX_SOURCE
CLEANFILES+=	t1 t2

.PATH:		${.CURDIR}/..

regress:
	@echo Testing posix rename with -D_POSIX_SOURCE
	./prename

.include <bsd.prog.mk>
