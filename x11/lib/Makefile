#	$NetBSD: Makefile,v 1.2 2004/02/06 14:55:48 mrg Exp $

SUBDIR=	X11 ICE Xau .WAIT \
	SM Xext .WAIT \
	Xt Xpm Xrender GL .WAIT \
	Xmu dps .WAIT \
	FS GLU GLw OSmesa XRes XTrap Xaw Xaw6 Xcursor Xdmcp Xext Xft Xft1 \
	Xfontcache Xi Xinerama Xmuu Xp Xrandr Xrender Xss Xtst Xv XvMC \
	Xxf86dga Xxf86misc Xxf86vm dpstk expat font fontconfig fontenc \
	freetype2 lbxutil psres oldX xkbfile xkbui xtrans

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR=+xlibi18n
.endif

.include <bsd.subdir.mk>
