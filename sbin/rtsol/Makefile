# $NetBSD: Makefile,v 1.12 2003/09/06 12:56:43 itojun Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	rtsol
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DUSE_RTSOCK

#MAN=	rtsold.8

.PATH:	${NETBSDSRCDIR}/usr.sbin/rtsold

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
