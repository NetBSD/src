#	$NetBSD: Makefile,v 1.10.42.1 2012/03/02 18:27:56 riz Exp $

# we encode the inter-library dependencies in the SUBDIR order:
# each library depends on at least one library in the previous group.

SUBDIR=\
	X11 ICE Xau freetype2 FS fontenc lbxutil psres xtrans Xdmcp \
	 .WAIT \
	SM Xext oldX fontconfig Xmuu Xpm \
	 .WAIT \
	Xt Xrender GL Xfontcache font xkbfile XRes Xi Xinerama Xp Xss Xtst \
	Xv XvMC Xxf86dga Xxf86misc Xxf86vm dmx \
	 .WAIT \
	Xmu dps XTrap GLU GLw OSmesa Xcursor Xrandr Xft Xft1 xkbui \
	 .WAIT \
	Xaw Xaw6 dpstk
 

.include <bsd.own.mk>

.if ${MKPIC} != "no"
SUBDIR+= xlibi18n
.endif

# Cache .OBJDIRS of LIBDPLIBS commonly used in SUBDIRs.
#
LIBDPLIBS=\
	X11	${.CURDIR}/X11/dynamic \
	Xext	${.CURDIR}/Xext \
	ICE	${.CURDIR}/ICE \
	SM	${.CURDIR}/SM \
	Xt	${.CURDIR}/Xt

libinstall::


.include <bsd.lib.mk>
.include <bsd.subdir.mk>
