#	$NetBSD: Makefile,v 1.7.24.1 2021/06/02 14:46:23 cjep Exp $

.include <bsd.own.mk>

PROG=	xrandr

LDADD+=	${XLIBLDADD_XRANDR} -lm
DPADD+=	${XLIBDPADD_XRANDR} ${LIBM}

CPPFLAGS+=	-DVERSION=\"1.4.0\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-parentheses
COPTS.xrandr.c += -Wno-stack-protector

.include <bsd.x11.mk>
.include <bsd.prog.mk>
