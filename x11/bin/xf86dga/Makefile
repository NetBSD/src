#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:19 lukem Exp $

.include <bsd.own.mk>

PROG=	dga

LDADD+=	-lXxf86dga -lXext -lX11
DPADD+=	${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/xf86dga

.include <bsd.x11.mk>
.include <bsd.prog.mk>
