#	$NetBSD: Makefile,v 1.10 2002/01/29 10:20:31 tv Exp $

PROG=	makewhatis
MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
.endif
