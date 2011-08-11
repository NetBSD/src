#	$NetBSD: Makefile,v 1.5 2011/08/11 23:15:36 joerg Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11 -lm
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.3.1\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-parentheses

.include <bsd.x11.mk>
.include <bsd.prog.mk>
