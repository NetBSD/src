#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8.4.1 2012/04/17 00:09:46 yamt Exp $

.include <bsd.own.mk>

WARNS ?= 4
PROG=	edquota
SRCS=	edquota.c
MAN=	edquota.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	printquota.c

.include <bsd.prog.mk>
