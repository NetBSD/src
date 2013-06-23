#	$NetBSD: Makefile,v 1.1.1.1.28.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xev

LDADD+=	-lX11 -lXrandr
DPADD+=	${LIBX11} ${LIBXRANDR}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
