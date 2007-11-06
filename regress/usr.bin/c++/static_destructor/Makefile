# $NetBSD: Makefile,v 1.1.2.2 2007/11/06 23:12:27 matt Exp $

PROG_CXX=	static_destructor
NOMAN=		# defined

regress:
	./${PROG} > out && diff out ${.CURDIR}/expected

CLEANFILES+=	out

.include <bsd.prog.mk>
