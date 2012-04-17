#	$NetBSD: Makefile,v 1.2.6.1 2012/04/17 00:09:54 yamt Exp $

PROG=	tprof
MAN=	tprof.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
