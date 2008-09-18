#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:29 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xev

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
