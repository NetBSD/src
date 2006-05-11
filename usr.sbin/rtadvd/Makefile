# $NetBSD: Makefile,v 1.9 2006/05/11 07:20:19 mrg Exp $

.include <bsd.own.mk>

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.dump.c=-fno-strict-aliasing
.endif
