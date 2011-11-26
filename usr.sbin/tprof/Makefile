#	$NetBSD: Makefile,v 1.3 2011/11/26 05:02:44 yamt Exp $

PROG=	tprof
MAN=	tprof.8

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
