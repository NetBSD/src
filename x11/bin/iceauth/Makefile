#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:05 lukem Exp $

.include <bsd.own.mk>

PROG=	iceauth
SRCS=	iceauth.c process.c

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
