#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:11 tv Exp $

PROG=		prename
NOMAN=		# defined
CPPFLAGS+=	-DBSD_RENAME
CLEANFILES+=	t1 t2

.PATH:		${.CURDIR}/..

regress:
	@echo Testing BSD rename
	./prename

.include <bsd.prog.mk>
