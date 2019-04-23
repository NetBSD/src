#	$NetBSD: Makefile,v 1.12.6.1 2019/04/23 10:24:54 martin Exp $

.include <bsd.own.mk>

# XXX when updating to a new xkeyboard-config, regenerate with xsltproc
MAN=		xkeyboard-config.7

SUBDIR=		compat geometry keycodes rules symbols types

NOOBJ=		# defined

PKGCONFIG=	xkeyboard-config

X11EXTRAMANTRANSFORMS+= \
	__xkb_base__		${X11LIBDIR}/xkb

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
