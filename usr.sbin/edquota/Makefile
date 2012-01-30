#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 2012/01/30 19:16:36 dholland Exp $

.include <bsd.own.mk>

WARNS ?= 4
PROG=	edquota
SRCS=	edquota.c
MAN=	edquota.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBPROP} ${LIBRPCSVC}
LDADD=	-lquota -lprop -lrpcsvc

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	printquota.c quotautil.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota1_subr.c

.include <bsd.prog.mk>
