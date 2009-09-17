#	$NetBSD: Makefile,v 1.1.1.1.8.1 2009/09/17 04:24:29 snj Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11 -lm
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.3.1\"

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
