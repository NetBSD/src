#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:33 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xvinfo

LDADD+=	-lXv -lXext -lX11
DPADD+=	${LIBXV} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
