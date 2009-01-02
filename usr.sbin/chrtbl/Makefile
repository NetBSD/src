#	$NetBSD: Makefile,v 1.8 2009/01/02 00:20:23 tnozaki Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.chrtbl.c+=	-Wno-pointer-sign
.endif
