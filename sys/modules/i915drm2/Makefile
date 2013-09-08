# $NetBSD: Makefile,v 1.1.2.10 2013/09/08 15:59:47 riastradh Exp $

.include "../Makefile.inc"
.include "../drm2/Makefile.inc"

CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/drm/i915

.PATH:	${S}/external/bsd/drm2/i915drm
.PATH:	${S}/external/bsd/drm2/dist/drm/i915

KMOD=	i915drm2
IOCONF=	i915drm.ioconf

SRCS+=	dvo_ch7017.c
SRCS+=	dvo_ch7xxx.c
SRCS+=	dvo_ivch.c
SRCS+=	dvo_ns2501.c
SRCS+=	dvo_sil164.c
SRCS+=	dvo_tfp410.c
#SRCS+=	i915_debugfs.c		# XXX No debugfs in NetBSD.
SRCS+=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_gem.c
SRCS+=	i915_gem_context.c
SRCS+=	i915_gem_debug.c
#SRCS+=	i915_gem_dmabuf.c
SRCS+=	i915_gem_evict.c
SRCS+=	i915_gem_execbuffer.c
SRCS+=	i915_gem_gtt.c
SRCS+=	i915_gem_stolen.c
SRCS+=	i915_gem_tiling.c
#SRCS+=	i915_ioc32.c
SRCS+=	i915_irq.c
SRCS+=	i915_suspend.c
SRCS+=	i915_sysfs.c		# XXX No sysfs in NetBSD.
SRCS+=	intel_bios.c
SRCS+=	intel_crt.c
SRCS+=	intel_ddi.c
SRCS+=	intel_display.c
SRCS+=	intel_dp.c
SRCS+=	intel_dvo.c
SRCS+=	intel_fb.c		# XXX Rewrite for wscons.
SRCS+=	intel_hdmi.c
SRCS+=	intel_i2c.c
SRCS+=	intel_lvds.c
SRCS+=	intel_modes.c
SRCS+=	intel_opregion.c
SRCS+=	intel_overlay.c
SRCS+=	intel_panel.c
SRCS+=	intel_pm.c		# XXX overridden for now
SRCS+=	intel_ringbuffer.c	# XXX overridden for now
SRCS+=	intel_sdvo.c
SRCS+=	intel_sprite.c
SRCS+=	intel_tv.c

SRCS+=	i915_module.c
SRCS+=	i915_pci.c
SRCS+=	intel_gtt.c

COPTS.intel_pm.c+=	-Wno-shadow

.include <bsd.kmodule.mk>
