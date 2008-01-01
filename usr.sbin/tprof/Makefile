#	$NetBSD: Makefile,v 1.1 2008/01/01 21:33:26 yamt Exp $

PROG=	tprof
NOMAN=	

WARNS?=	4

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
