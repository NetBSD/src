#	$NetBSD: Makefile,v 1.41 2022/07/15 04:18:55 mrg Exp $

.include <bsd.own.mk>

SUBDIR=		common ddc dixmods doc dri dri2 i2c loader \
		xf86modes exa \
		xorgos ramdac shadowfb vgahw \
		x86emu xf86config xkb \
		drivers

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    (${MACHINE_CPU} == "powerpc" && ${MACHINE} != "evbppc")
SUBDIR+=	int10 
.endif

.if ${HAVE_XORG_GLAMOR} != "no"
SUBDIR+=	glamor_egl
.endif

SUBDIR+=	.WAIT utils

SUBDIR+=	.WAIT Xorg

.include <bsd.subdir.mk>
