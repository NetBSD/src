#	$NetBSD: Makefile,v 1.6 2018/03/14 09:09:46 mrg Exp $

# *proto packages for xorg

.if 0
SUBDIR=	xorgproto
.else
SUBDIR=	xproto fontsproto bigreqsproto kbproto xcmiscproto xf86bigfontproto \
	fixesproto inputproto scrnsaverproto xextproto evieext xineramaproto \
	resourceproto recordproto videoproto xf86dgaproto xf86miscproto \
	compositeproto damageproto trapproto glproto randrproto \
	xf86driproto xf86vidmodeproto renderproto dri2proto dri3proto \
	presentproto xproxymanagementprotocol
.endif
SUBDIR+= xbitmaps xcb-proto .WAIT xtrans

.include <bsd.subdir.mk>
