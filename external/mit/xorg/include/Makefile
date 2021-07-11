#	$NetBSD: Makefile,v 1.10 2021/07/11 20:52:06 mrg Exp $

# *proto packages for xorg

.include <bsd.own.mk>

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "evbarm"
SUBDIR+=	libvdpau
.endif
SUBDIR+=	KHR 
SUBDIR+=	xorgproto xbitmaps xcb-proto
SUBDIR+=	.WAIT xtrans

.include <bsd.subdir.mk>
