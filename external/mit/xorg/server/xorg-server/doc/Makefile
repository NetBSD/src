#	$NetBSD: Makefile,v 1.2 2008/10/14 23:37:19 cube Exp $

.include <bsd.own.mk>

.include "../dix/Makefile.common"

.PATH: ${X11SRCDIR.xorg-server}/doc
MAN=	SecurityPolicy.5 Xserver.1
X11EXTRAMANDEFS+=	-e 's%__default_font_path__%${X11DEFAULTFONTPATH:ts,:S/,/, /g}%g'

.include <bsd.x11.mk>
.include <bsd.man.mk>
