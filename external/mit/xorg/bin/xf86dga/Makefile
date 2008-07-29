#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	dga

LDADD+=	-lXxf86dga -lXext -lX11
DPADD+=	${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xf86dga}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
