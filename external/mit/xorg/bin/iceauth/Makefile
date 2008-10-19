#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:57 haad Exp $

.include <bsd.own.mk>

PROG=	iceauth
SRCS=	iceauth.c process.c

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
