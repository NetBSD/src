#	$NetBSD: Makefile,v 1.7 2016/06/07 23:33:45 christos Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	-lXrandr -lXrender -lXext -lX11 -lm
DPADD+=	${LIBXRANDR} ${LIBXRENDER} ${LIBXEXT} ${LIBX11} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.4.0\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-parentheses
COPTS.xrandr.c += -Wno-stack-protector

.include <bsd.x11.mk>
.include <bsd.prog.mk>
