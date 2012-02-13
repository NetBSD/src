#	$NetBSD: Makefile,v 1.1 2012/02/13 01:35:09 dholland Exp $

PROG=	quotarestore
SRCS=	quotarestore.c 
MAN=	quotarestore.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
