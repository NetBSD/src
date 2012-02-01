#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.6 2012/02/01 17:53:01 dholland Exp $

.include <bsd.own.mk>
PROG=	quotactl
SRCS=	quotactl.c proplib-interpreter.c quotaprop.c
MAN=	quotactl.8

DPADD=	${LIBPROP} ${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lprop -lquota -lrpcsvc

.include <bsd.prog.mk>
