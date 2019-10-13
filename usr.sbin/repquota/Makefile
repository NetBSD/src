#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.11 2019/10/13 07:28:20 mrg Exp $

WARNS ?= 4
.include <bsd.own.mk>
PROG=	repquota
SRCS=	repquota.c 
MAN=	repquota.8

LINKS+=	${BINDIR}/repquota ${BINDIR}/quotadump
MLINKS+=repquota.8 quotadump.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	printquota.c

COPTS.printquota.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
