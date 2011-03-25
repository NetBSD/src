#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.3 2011/03/25 10:30:35 bouyer Exp $

.include <bsd.own.mk>
PROG=	quotactl
SRCS=	quotactl.c
MAN=	quotactl.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBPROP}
LDADD=	-lquota -lrpcsvc -lprop

.include <bsd.prog.mk>
