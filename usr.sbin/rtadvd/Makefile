# $NetBSD: Makefile,v 1.3 2000/05/23 11:37:58 itojun Exp $

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

.include <bsd.prog.mk>
