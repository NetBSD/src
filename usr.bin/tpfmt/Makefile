#	$NetBSD: Makefile,v 1.2 2010/11/24 13:17:56 christos Exp $

PROG=	tpfmt
NOMAN=	

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/
SRCS=	tpfmt.c sym.c

LDADD+= -lpthread
LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBPTHREAD}
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
