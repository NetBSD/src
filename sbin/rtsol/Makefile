# $NetBSD: Makefile,v 1.2 1999/09/03 05:25:44 itojun Exp $

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c

CPPFLAGS+=-DINET6

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

#MAN=	rtsold.8
NOMAN=	yes

.PATH:	${.CURDIR}/../../usr.sbin/rtsold

.include <bsd.prog.mk>
