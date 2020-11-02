#	$NetBSD: Makefile,v 1.14 2020/11/02 00:25:46 mrg Exp $

.include <bsd.own.mk>

# XXX when updating to a new xkeyboard-config, regenerate with xsltproc
MAN=		xkeyboard-config.7

SUBDIR=		compat geometry keycodes rules symbols types

NOOBJ=		# defined

PKGCONFIG=	xkeyboard-config

X11EXTRAMANTRANSFORMS+= \
	@xkb_base@		${X11LIBDIR}/xkb

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
