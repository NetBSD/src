# $NetBSD: Makefile,v 1.2.4.1 1999/12/27 18:38:06 wrstuden Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c

CPPFLAGS+=-DINET6

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

.include <bsd.prog.mk>
