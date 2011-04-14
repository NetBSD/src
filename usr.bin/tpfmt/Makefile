#	$NetBSD: Makefile,v 1.3 2011/04/14 16:25:58 yamt Exp $

PROG=	tpfmt
NOMAN=	

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/
SRCS=	tpfmt.c sym.c

LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
