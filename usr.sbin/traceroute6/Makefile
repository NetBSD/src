# $NetBSD: Makefile,v 1.11 2020/09/06 07:20:33 mrg Exp $

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

CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
