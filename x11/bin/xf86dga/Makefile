#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=	dga

LDADD+=	-lXxf86dga -lXext -lX11
DPADD+=	${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/xf86dga

.include <bsd.prog.mk>
.include <bsd.x11.mk>
