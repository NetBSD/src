# $NetBSD: Makefile,v 1.6 2001/05/15 17:07:49 kleink Exp $

.include <bsd.own.mk>

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lcompat
DPADD+=	${LIBCOMPAT}

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.include <bsd.prog.mk>
