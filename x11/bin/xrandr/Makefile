#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:34 lukem Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
