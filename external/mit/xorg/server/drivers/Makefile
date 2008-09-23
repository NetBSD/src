#	$NetBSD: Makefile,v 1.11 2008/09/23 11:33:16 rtr Exp $

SUBDIR=		xf86-input-keyboard xf86-input-void
SUBDIR+=	xf86-input-mouse
.if ${MACHINE_ARCH} == "i368" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	xf86-input-vmmouse
.endif

SUBDIR+=	xf86-video-apm xf86-video-ark xf86-video-ast
SUBDIR+=	xf86-video-ati xf86-video-chips xf86-video-cirrus
SUBDIR+=	xf86-video-cirrus_alpine xf86-video-cirrus_laguna
SUBDIR+=	xf86-video-cyrix xf86-video-glint xf86-video-i128
SUBDIR+=	xf86-video-i740 xf86-video-imstt xf86-video-intel
SUBDIR+=	xf86-video-mga xf86-video-neomagic xf86-video-nsc
SUBDIR+=	xf86-video-nv xf86-video-mach64
#SUBDIR+=	xf86-video-radeonhd xf86-video-rendition
SUBDIR+=	xf86-video-radeon
SUBDIR+=	xf86-video-s3 xf86-video-s3virge xf86-video-savage
SUBDIR+=	xf86-video-siliconmotion xf86-video-sis xf86-video-tdfx
SUBDIR+=	xf86-video-tga xf86-video-trident xf86-video-tseng
SUBDIR+=	xf86-video-vesa xf86-video-vga xf86-video-via
SUBDIR+=	xf86-video-vmware #xf86-video-wsfb

.include <bsd.subdir.mk>
