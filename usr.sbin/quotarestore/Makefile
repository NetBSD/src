#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:09:52 yamt Exp $

PROG=	quotarestore
SRCS=	quotarestore.c 
MAN=	quotarestore.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
