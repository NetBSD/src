# $NetBSD: Makefile,v 1.1 2003/12/04 18:14:59 drochner Exp $

PROG=	sntp
SRCS=	internet.c main.c socket.c timing.c unix.c

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist/ntp

LDADD+=	-lm
DPADD+=	${LIBM}

.PATH:	${DIST}/sntp

.include <bsd.prog.mk>
