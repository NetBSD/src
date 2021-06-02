#	$NetBSD: Makefile,v 1.5.36.1 2021/06/02 14:46:23 cjep Exp $

.include <bsd.own.mk>

PROG=	xprop
SRCS=	xprop.c dsimple.c clientwin.c

CPPFLAGS+=-DHAVE_WCHAR_H -DHAVE_WCTYPE_H -DNO_WIDEC_H
CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include

LDADD+=	${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XMUU}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

COPTS.xprop.c+=	-Wno-error	# uses XKeycodeToKeysym

.include <bsd.x11.mk>
.include <bsd.prog.mk>
