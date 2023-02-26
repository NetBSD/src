#	$NetBSD: Makefile,v 1.9 2023/02/26 20:19:05 mrg Exp $

DRIVER=		xf86-video-trident
DRIVER_NAME=	trident_drv

SRCS=		blade_exa.c blade_xaa.c image_xaa.c
SRCS+=		tgui_xaa.c trident_bank.c trident_dac.c
SRCS+=		trident_dga.c trident_driver.c
SRCS+=		trident_i2c.c trident_pll.c trident_shadow.c
SRCS+=		trident_tv.c trident_video.c tvga_dac.c
SRCS+=		xp_xaa.c xp4_exa.c xp4_xaa.c

MAN=		trident.4

#COPTS.trident_driver.c=	-Wno-error	# XXX xf86dgastr.h

CWARNFLAGS.clang+=	-Wno-return-type -Wno-empty-body

.include "../Makefile.xf86-driver"
