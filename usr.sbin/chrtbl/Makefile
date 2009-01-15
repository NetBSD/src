#	$NetBSD: Makefile,v 1.7.4.1 2009/01/15 03:24:11 snj Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.chrtbl.c+=	-Wno-pointer-sign
.endif
