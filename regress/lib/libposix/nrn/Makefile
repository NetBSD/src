#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:49 lukem Exp $

NOMAN=		# defined

PROG=		prename
CPPFLAGS+=	-DBSD_RENAME
CLEANFILES+=	t1 t2

.PATH:		${.CURDIR}/..

regress:
	@echo Testing BSD rename
	./prename

.include <bsd.prog.mk>
