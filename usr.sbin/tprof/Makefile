#	$NetBSD: Makefile,v 1.1.2.2 2008/01/09 02:02:33 matt Exp $

PROG=	tprof
NOMAN=	

WARNS?=	4

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
