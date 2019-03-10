#	$NetBSD: Makefile,v 1.9 2019/03/10 13:52:11 mrg Exp $

# *proto packages for xorg

.include <bsd.own.mk>

.if ${HAVE_MESA_VER} == 18
SUBDIR=		KHR 
.endif
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "evbarm"
SUBDIR+=	libvdpau
.endif
SUBDIR+=	xorgproto xbitmaps xcb-proto
SUBDIR+=	.WAIT xtrans

.include <bsd.subdir.mk>
