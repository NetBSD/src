# $NetBSD: Makefile,v 1.4 2001/01/21 15:01:37 itojun Exp $

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf

.include <bsd.prog.mk>
