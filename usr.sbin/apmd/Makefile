#	$NetBSD: Makefile,v 1.8 2002/06/18 05:24:44 itojun Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc" || ${MACHINE} == "macppc"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
