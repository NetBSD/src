#	$NetBSD: Makefile,v 1.4 2006/05/11 23:16:29 mrg Exp $

SRCS=	atactl.c

PROG=	atactl
MAN=	atactl.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif
