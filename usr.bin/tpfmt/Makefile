#	$NetBSD: Makefile,v 1.1 2010/11/23 20:48:40 yamt Exp $

PROG=	tpfmt
NOMAN=	

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/
SRCS=	tpfmt.c sym.c

LDADD+= -lpthread
LDADD+= -lelf
DPADD+= ${LIBPTHREAD}
DPADD+= ${LIBELF}

.include <bsd.own.mk>
.include <bsd.prog.mk>
