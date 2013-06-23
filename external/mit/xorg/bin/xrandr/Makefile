#	$NetBSD: Makefile,v 1.5.8.1 2013/06/23 06:28:42 tls Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11 -lm
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.4.0\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.x11.mk>
.include <bsd.prog.mk>
