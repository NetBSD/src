#	$NetBSD: Makefile,v 1.11 2005/03/16 01:44:46 xtraeme Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

WARNS=  3

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
