#	$NetBSD: Makefile,v 1.36 2016/11/22 10:34:57 snj Exp $

SUBDIR=		common ddc dixmods doc dri dri2 i2c loader \
		xf86modes exa \
		xorgos ramdac shadowfb vbe vgahw \
		x86emu xf86config \
		drivers

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "powerpc"
SUBDIR+=	int10 
.endif

SUBDIR+=	.WAIT utils

SUBDIR+=	.WAIT Xorg

.include <bsd.subdir.mk>
