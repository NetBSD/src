#	$NetBSD: Makefile,v 1.9 2002/10/14 06:31:38 takemura Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcmips"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
