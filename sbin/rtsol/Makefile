# $NetBSD: Makefile,v 1.8 2002/05/31 10:22:11 itojun Exp $

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS -DHAVE_ARC4RANDOM

#MAN=	rtsold.8
NOMAN=	# defined

.PATH:	${.CURDIR}/../../usr.sbin/rtsold

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
