# $NetBSD: Makefile,v 1.2.4.1 1999/12/27 18:30:33 wrstuden Exp $

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c

CPPFLAGS+=-DINET6

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

#MAN=	rtsold.8
NOMAN=	yes

.PATH:	${.CURDIR}/../../usr.sbin/rtsold

.include <bsd.prog.mk>
