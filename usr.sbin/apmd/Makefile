#	$NetBSD: Makefile,v 1.10 2004/07/04 15:55:35 uch Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
