#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 2011/03/24 17:05:47 bouyer Exp $

WARNS ?= 4
.include <bsd.own.mk>
PROG=	repquota
SRCS=	repquota.c 
MAN=	repquota.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBPROP} ${LIBRPCSVC}
LDADD=	-lquota -lprop -lrpcsvc

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	printquota.c quotautil.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota1_subr.c

.include <bsd.prog.mk>
