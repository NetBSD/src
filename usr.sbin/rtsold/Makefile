# $NetBSD: Makefile,v 1.6 2001/07/09 06:04:28 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

.include <bsd.prog.mk>
