#	$NetBSD: Makefile,v 1.4 2011/11/26 16:15:09 yamt Exp $

PROG=	tpfmt

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/
SRCS=	tpfmt.c sym.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
