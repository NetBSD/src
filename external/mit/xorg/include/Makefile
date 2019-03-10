#	$NetBSD: Makefile,v 1.8 2019/03/10 11:18:02 mrg Exp $

# *proto packages for xorg

.include <bsd.own.mk>

.if ${HAVE_MESA_VER} == 18
SUBDIR=		KHR 
.endif
SUBDIR+=	libvdpau xorgproto xbitmaps xcb-proto
SUBDIR+=	.WAIT xtrans

.include <bsd.subdir.mk>
