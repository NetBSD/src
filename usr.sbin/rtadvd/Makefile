# $NetBSD: Makefile,v 1.13 2009/04/22 15:23:07 lukem Exp $

WARNS?=	2	# XXX -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?= yes	# network server

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

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4)
COPTS.dump.c=-fno-strict-aliasing
.endif
