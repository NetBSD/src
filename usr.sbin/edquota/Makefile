#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.10 2012/01/30 19:19:20 dholland Exp $

.include <bsd.own.mk>

WARNS ?= 4
PROG=	edquota
SRCS=	edquota.c
MAN=	edquota.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBPROP} ${LIBRPCSVC}
LDADD=	-lquota -lprop -lrpcsvc

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	printquota.c

.include <bsd.prog.mk>
