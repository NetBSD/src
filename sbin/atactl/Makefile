#	$NetBSD: Makefile,v 1.5 2008/07/21 11:38:44 gmcgarry Exp $

SRCS=	atactl.c

PROG=	atactl
MAN=	atactl.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.atactl.c	+=	-Wno-pointer-sign
.endif
