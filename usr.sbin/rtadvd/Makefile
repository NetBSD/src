# $NetBSD: Makefile,v 1.2 1999/07/03 06:26:26 itojun Exp $

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

.include <bsd.prog.mk>
