#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:49 lukem Exp $

NOMAN=		# defined

PROG=		prename
CLEANFILES+=	t1 t2
DPADD+=		${LIBPOSIX}
LDADD+=		-lposix

.PATH:		${.CURDIR}/..

regress:
	@echo Testing posix rename with -lposix
	./prename

.include <bsd.prog.mk>
