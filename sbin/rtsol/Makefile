# $NetBSD: Makefile,v 1.7 2001/12/12 01:48:43 tv Exp $

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS

#MAN=	rtsold.8
NOMAN=	# defined

.PATH:	${.CURDIR}/../../usr.sbin/rtsold

.include <bsd.prog.mk>
