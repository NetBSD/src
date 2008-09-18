#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:29 wrstuden Exp $

.include <bsd.own.mk>

PROG=	dga

LDADD+=	-lXxf86dga -lXext -lX11
DPADD+=	${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xf86dga}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
