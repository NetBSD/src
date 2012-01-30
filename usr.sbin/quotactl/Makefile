#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.5 2012/01/30 19:31:31 dholland Exp $

.include <bsd.own.mk>
PROG=	quotactl
SRCS=	quotactl.c proplib-interpreter.c quotaprop.c
MAN=	quotactl.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBPROP}
LDADD=	-lquota -lrpcsvc -lprop

.include <bsd.prog.mk>
