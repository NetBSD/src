# $NetBSD: Makefile,v 1.5 2000/02/25 09:19:07 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

.include <bsd.prog.mk>
