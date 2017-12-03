# $NetBSD: Makefile,v 1.4.4.3 2017/12/03 11:38:54 jdolecek Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/drm/i915
CPPFLAGS+=	-I${S}/external/bsd/drm2/i915drm

.PATH:	${S}/external/bsd/drm2/i915drm
.PATH:	${S}/external/bsd/drm2/dist/drm/i915

KMOD=	i915drmkms
IOCONF=	i915drmkms.ioconf
MKLDSCRIPT=yes

SRCS+=	dvo_ch7017.c
SRCS+=	dvo_ch7xxx.c
SRCS+=	dvo_ivch.c
SRCS+=	dvo_ns2501.c
SRCS+=	dvo_sil164.c
SRCS+=	dvo_tfp410.c
SRCS+=	i915_cmd_parser.c
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
SRCS+=	i915_gpu_error.c
#SRCS+=	i915_ioc32.c
SRCS+=	i915_irq.c
SRCS+=	i915_params.c
SRCS+=	i915_suspend.c
SRCS+=	i915_sysfs.c		# XXX No sysfs in NetBSD.
SRCS+=	i915_ums.c
SRCS+=	intel_acpi.c
SRCS+=	intel_bios.c
SRCS+=	intel_crt.c
SRCS+=	intel_ddi.c
SRCS+=	intel_display.c
SRCS+=	intel_dp.c
SRCS+=	intel_dsi.c
SRCS+=	intel_dsi_cmd.c
SRCS+=	intel_dsi_pll.c
SRCS+=	intel_dvo.c
SRCS+=	intel_fbdev.c
SRCS+=	intel_hdmi.c
SRCS+=	intel_i2c.c
SRCS+=	intel_lvds.c
SRCS+=	intel_modes.c
SRCS+=	intel_opregion.c
SRCS+=	intel_overlay.c
SRCS+=	intel_panel.c
SRCS+=	intel_pm.c
SRCS+=	intel_ringbuffer.c
SRCS+=	intel_sdvo.c
SRCS+=	intel_sideband.c
SRCS+=	intel_sprite.c
SRCS+=	intel_tv.c
SRCS+=	intel_uncore.c

SRCS+=	i915_module.c
SRCS+=	i915_pci.c
SRCS+=	intel_gtt.c
SRCS+=	intelfb.c

COPTS.i915_drv.c+=	${${ACTIVE_CC} == "gcc":? -Wno-override-init :}
COPTS.i915_drv.c+=	${${ACTIVE_CC} == "clang":? -Wno-initializer-overrides :}

COPTS.intel_display.c+=	-Wno-shadow -Wno-conversion
COPTS.intel_panel.c+=	-Wno-shadow
COPTS.intel_pm.c+=	-Wno-shadow

CPPFLAGS+=		-DCONFIG_DRM_I915_FBDEV=1
CPPFLAGS+=		-DCONFIG_DRM_I915_PRELIMINARY_HW_SUPPORT=0

CPPFLAGS+=		-DNACPICA=1
CPPFLAGS+=		-DNVGA=1

.include <bsd.kmodule.mk>
