#	$NetBSD: Makefile,v 1.6.4.1 1999/12/27 18:37:21 wrstuden Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
