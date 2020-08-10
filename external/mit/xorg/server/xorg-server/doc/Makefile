#	$NetBSD: Makefile,v 1.6 2020/08/10 09:23:37 mrg Exp $

.include <bsd.own.mk>

.include "../dix/Makefile.common"

.PATH: ${X11SRCDIR.xorg-server}/doc
.PATH: ${X11SRCDIR.xorg-server}/man
MAN=	Xserver.1
X11EXTRAMANDEFS+=	-e 's%@default_font_path@%${X11DEFAULTFONTPATH:ts,:S/,/, /g}%g'

.include <bsd.x11.mk>
.include <bsd.man.mk>
