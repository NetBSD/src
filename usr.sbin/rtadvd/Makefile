# $NetBSD: Makefile,v 1.14.4.1 2011/06/23 14:20:50 cherry Exp $

WARNS?=	2	# XXX -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	rtadvd
SRCS=	rtadvd.c rrenum.c advcap.c if.c config.c timer.c dump.c

CPPFLAGS+=-DINET6 -DROUTEINFO
MAN=	rtadvd.8 rtadvd.conf.5
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/examples/rtadvd
FILES=	rtadvd.conf
.endif

.include <bsd.prog.mk>

.if defined(HAVE_GCC)
COPTS.dump.c=-fno-strict-aliasing
.endif
