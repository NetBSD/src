# $NetBSD: Makefile,v 1.7 2002/05/21 23:35:18 itojun Exp $

.include <bsd.own.mk>

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lcompat -lutil
DPADD+=	${LIBCOMPAT} ${LIBUTIL}

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.include <bsd.prog.mk>
