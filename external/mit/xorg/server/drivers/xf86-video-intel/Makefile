#	$NetBSD: Makefile,v 1.5.8.1 2009/09/17 04:24:35 snj Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	intel_drv

SRCS=		drmmode_display.c i810_accel.c i810_cursor.c i810_dga.c
SRCS+=		i810_driver.c i810_io.c i810_memory.c i810_video.c
SRCS+=		i810_wmark.c i830_3d.c i830_accel.c i830_bios.c
SRCS+=		i830_batchbuffer.c i830_crt.c i830_cursor.c i830_debug.c
SRCS+=		i830_display.c i830_quirks.c i830_driver.c i830_dvo.c
SRCS+=		i830_hdmi.c i830_i2c.c i830_io.c i830_lvds.c i830_memory.c
SRCS+=		i830_modes.c i830_video.c i830_sdvo.c i830_tv.c
SRCS+=		i915_3d.c i915_video.c i965_video.c 
SRCS+=		i830_xaa.c i830_render.c i915_render.c i965_render.c
SRCS+=		i830_dri.c i830_exa.c
MAN=		intel.4

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm
CPPFLAGS+=	-DI830_XV -DI830_USE_XAA -DI830_USE_EXA

LDADD+=		-ldrm_intel

.include "../Makefile.xf86-driver"

SUBDIR= ch7017 ch7xxx ivch sil164 tfp410

.include <bsd.subdir.mk>
