# $NetBSD: Makefile,v 1.13 2004/01/03 01:40:31 itojun Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c rtsock.c

CPPFLAGS+=-DSMALL -DUSE_RTSOCK

#MAN=	rtsold.8

.PATH:	${NETBSDSRCDIR}/usr.sbin/rtsold

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
