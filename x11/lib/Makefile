#	$NetBSD: Makefile,v 1.7 2005/07/19 21:14:39 christos Exp $

SUBDIR=	X11 ICE Xau .WAIT \
	SM Xext .WAIT \
	Xt Xpm Xrender GL .WAIT \
	Xmu dps .WAIT \
	FS GLU GLw OSmesa XRes XTrap Xaw Xaw6 Xcursor Xdmcp Xft Xft1 \
	Xfontcache Xi Xinerama Xmuu Xp Xrandr Xss Xtst Xv XvMC \
	Xxf86dga Xxf86misc Xxf86vm dmx dpstk expat font fontconfig fontenc \
	freetype2 lbxutil psres oldX xkbfile xkbui xtrans

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+= xlibi18n
.endif

# Cache .OBJDIRS of LIBDPLIBS commonly used in SUBDIRs.
#
LIBDPLIBS=	X11	${.CURDIR}/X11/dynamic \
		Xext	${.CURDIR}/Xext \
		ICE	${.CURDIR}/ICE \
		SM	${.CURDIR}/SM \
		Xt	${.CURDIR}/Xt

libinstall::


#.include <bsd.lib.mk>
.include <bsd.subdir.mk>
