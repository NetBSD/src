# $NetBSD: Makefile,v 1.11 2003/05/15 14:44:58 itojun Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_ARC4RANDOM -DUSE_RTSOCK

#MAN=	rtsold.8

.PATH:	${NETBSDSRCDIR}/usr.sbin/rtsold

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
