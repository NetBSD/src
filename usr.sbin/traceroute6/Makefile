# $NetBSD: Makefile,v 1.12 2021/02/03 06:42:50 roy Exp $

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
