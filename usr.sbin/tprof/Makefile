#	$NetBSD: Makefile,v 1.1.14.1 2009/05/13 19:20:42 jym Exp $

PROG=	tprof
NOMAN=	

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
