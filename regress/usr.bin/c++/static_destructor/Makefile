# $NetBSD: Makefile,v 1.1 2007/09/17 17:37:48 drochner Exp $

PROG_CXX=	static_destructor
NOMAN=		# defined

regress:
	./${PROG} > out && diff out ${.CURDIR}/expected

CLEANFILES+=	out

.include <bsd.prog.mk>
