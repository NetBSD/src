# $NetBSD: Makefile,v 1.10 2002/08/19 10:17:00 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS -DHAVE_ARC4RANDOM -DUSE_RTSOCK

#MAN=	rtsold.8

.PATH:	${NETBSDSRCDIR}/usr.sbin/rtsold

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
