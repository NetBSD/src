#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.10 2012/02/01 17:53:01 dholland Exp $

.include <bsd.own.mk>
WARNS	?= 4

PROG=	quotaon
SRCS=	quotaon.c
MAN=	quotaon.8
MLINKS=	quotaon.8 quotaoff.8
LINKS=	${BINDIR}/quotaon ${BINDIR}/quotaoff

DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
