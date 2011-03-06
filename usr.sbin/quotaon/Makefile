#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.6 2011/03/06 17:08:43 bouyer Exp $

.include <bsd.own.mk>
PROG=	quotaon
SRCS=	quotaon.c
MAN=	quotaon.8
MLINKS=	quotaon.8 quotaoff.8
LINKS=	${BINDIR}/quotaon ${BINDIR}/quotaoff

CPPFLAGS+=-I${NETBSDSRCDIR}/sys
DPADD=	${LIBPROP}
LDADD=	-lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs
SRCS+=	quota2_prop.c

.include <bsd.prog.mk>
