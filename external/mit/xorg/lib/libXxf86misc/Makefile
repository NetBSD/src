#	$NetBSD: Makefile,v 1.3 2008/10/04 18:32:25 cube Exp $

.include <bsd.own.mk>

LIB=	Xxf86misc
.PATH:	${X11SRCDIR.${LIB}}/src ${X11SRCDIR.${LIB}}/man

SRCS=	XF86Misc.c

MAN=	XF86Misc.3
MLINKS=	XF86Misc.3 XF86MiscGetKbdSettings.3 \
	XF86Misc.3 XF86MiscGetMouseSettings.3 \
	XF86Misc.3 XF86MiscGetSaver.3 \
	XF86Misc.3 XF86MiscQueryExtension.3 \
	XF86Misc.3 XF86MiscQueryVersion.3 \
	XF86Misc.3 XF86MiscSetKbdSettings.3 \
	XF86Misc.3 XF86MiscSetMouseSettings.3 \
	XF86Misc.3 XF86MiscSetSaver.3

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
