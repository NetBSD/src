#	$NetBSD: Makefile,v 1.2.4.1 2009/09/17 04:24:38 snj Exp $

.include <bsd.own.mk>

.include "../dix/Makefile.common"

.PATH: ${X11SRCDIR.xorg-server}/doc
MAN=	Xserver.1
X11EXTRAMANDEFS+=	-e 's%__default_font_path__%${X11DEFAULTFONTPATH:ts,:S/,/, /g}%g'

.include <bsd.x11.mk>
.include <bsd.man.mk>
