#	$NetBSD: Makefile,v 1.5 2016/08/14 00:03:58 mrg Exp $

.include <bsd.own.mk>

.include "../dix/Makefile.common"

.PATH: ${X11SRCDIR.xorg-server}/doc
.PATH: ${X11SRCDIR.xorg-server}/man
MAN=	Xserver.1
X11EXTRAMANDEFS+=	-e 's%__default_font_path__%${X11DEFAULTFONTPATH:ts,:S/,/, /g}%g'

.include <bsd.x11.mk>
.include <bsd.man.mk>
