#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11 2012/04/07 05:07:32 christos Exp $

.include <bsd.own.mk>
WARNS	?= 4

PROG=	quotaon
SRCS=	quotaon.c
MAN=	quotaon.8
MLINKS=	quotaon.8 quotaoff.8
LINKS=	${BINDIR}/quotaon ${BINDIR}/quotaoff

DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lquota -lrpcsvc -lutil

.include <bsd.prog.mk>
