# $NetBSD: Makefile,v 1.10 2018/04/23 10:19:11 maxv Exp $

USE_FORT?= yes	# network client

TRACEROUTE?= ${.CURDIR}/../traceroute

SRCS=	traceroute6.c as.c
PROG=	traceroute6
MAN=	traceroute6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

CPPFLAGS+=-DHAVE_POLL
CPPFLAGS+=-I${TRACEROUTE}

.PATH: ${TRACEROUTE}

.include <bsd.prog.mk>
