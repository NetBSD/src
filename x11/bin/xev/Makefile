#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=	xev

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
