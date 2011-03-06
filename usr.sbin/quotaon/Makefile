#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.7 2011/03/06 23:24:33 christos Exp $

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
DPADD=	${LIBPROP}
LDADD=	-lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs
SRCS+=	quota2_prop.c

.include <bsd.prog.mk>
