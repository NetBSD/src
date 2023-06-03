#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.13 2023/06/03 09:09:21 lukem Exp $

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

COPTS.printquota.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
