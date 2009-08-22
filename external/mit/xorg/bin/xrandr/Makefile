#	$NetBSD: Makefile,v 1.3 2009/08/22 01:03:11 mrg Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11 -lm
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.3.1\"

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
