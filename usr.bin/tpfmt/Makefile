#	$NetBSD: Makefile,v 1.3.4.1 2012/04/17 00:09:40 yamt Exp $

PROG=	tpfmt

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/
SRCS=	tpfmt.c sym.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
