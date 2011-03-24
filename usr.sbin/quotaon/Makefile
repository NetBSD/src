#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 2011/03/24 17:05:47 bouyer Exp $

.include <bsd.own.mk>
WARNS	?= 4

PROG=	quotaon
SRCS=	quotaon.c
MAN=	quotaon.8
MLINKS=	quotaon.8 quotaoff.8
LINKS=	${BINDIR}/quotaon ${BINDIR}/quotaoff

.PATH:  ${NETBSDSRCDIR}/usr.bin/quota
SRCS+=  quotautil.c
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/quota
CPPFLAGS+=-I${NETBSDSRCDIR}/sys

DPADD=	${LIBQUOTA} ${LIBPROP} ${LIBRPCSVC}
LDADD=	-lquota -lprop -lrpcsvc

.include <bsd.prog.mk>
