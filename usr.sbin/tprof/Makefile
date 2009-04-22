#	$NetBSD: Makefile,v 1.2 2009/04/22 15:23:09 lukem Exp $

PROG=	tprof
NOMAN=	

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>
