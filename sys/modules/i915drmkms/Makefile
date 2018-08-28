# $NetBSD: Makefile,v 1.11 2018/08/28 03:48:38 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

KMOD=	i915drmkms
IOCONF=	i915drmkms.ioconf
MKLDSCRIPT=yes

.PATH:	${S}/external/bsd/drm2/i915drm

.PATH:	${S}/external/bsd/drm2/dist/drm/i915

CPPFLAGS+=	-I${S}/sys/modules/drmkms

CPPFLAGS+=	-I${S}/external/bsd/drm2/i915drm

CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/drm/i915

CPPFLAGS+=	-DCONFIG_DRM_I915_FBDEV=1
CPPFLAGS+=	-DCONFIG_DRM_I915_PRELIMINARY_HW_SUPPORT=0

# Substitute for _KERNEL_OPT flags.
CPPFLAGS+=	-DNACPICA=1
CPPFLAGS+=	-DNVGA=1

CWARNFLAGS+=	-Wno-shadow

COPTS.i915_drv.c+=	${${ACTIVE_CC} == "gcc":? -Wno-override-init :}
COPTS.i915_drv.c+=	${${ACTIVE_CC} == "clang":? -Wno-initializer-overrides :}

# NetBSD additions.
SRCS+=	i915_module.c
SRCS+=	i915_pci.c
SRCS+=	intel_gtt.c

# wsconsole framebuffer.
SRCS+=	intelfb.c

# Upstream sources.
SRCS+=	dvo_ch7017.c
SRCS+=	dvo_ch7xxx.c
SRCS+=	dvo_ivch.c
SRCS+=	dvo_ns2501.c
SRCS+=	dvo_sil164.c
SRCS+=	dvo_tfp410.c
SRCS+=	i915_cmd_parser.c
SRCS+=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_gem.c
SRCS+=	i915_gem_batch_pool.c
SRCS+=	i915_gem_context.c
SRCS+=	i915_gem_debug.c
#SRCS+=	i915_gem_dmabuf.c	# XXX drm prime
SRCS+=	i915_gem_evict.c
SRCS+=	i915_gem_execbuffer.c
SRCS+=	i915_gem_fence.c
SRCS+=	i915_gem_gtt.c
SRCS+=	i915_gem_render_state.c
SRCS+=	i915_gem_shrinker.c
SRCS+=	i915_gem_stolen.c
SRCS+=	i915_gem_tiling.c
SRCS+=	i915_gem_userptr.c
SRCS+=	i915_gpu_error.c
SRCS+=	i915_guc_submission.c
SRCS+=	i915_irq.c
SRCS+=	i915_params.c
SRCS+=	i915_suspend.c
SRCS+=	i915_sysfs.c
SRCS+=	i915_trace_points.c
SRCS+=	i915_vgpu.c
SRCS+=	intel_acpi.c
SRCS+=	intel_atomic.c
SRCS+=	intel_atomic_plane.c
SRCS+=	intel_audio.c
SRCS+=	intel_bios.c
SRCS+=	intel_crt.c
SRCS+=	intel_ddi.c
SRCS+=	intel_display.c
SRCS+=	intel_dp.c
SRCS+=	intel_dp_mst.c
SRCS+=	intel_dsi.c
#SRCS+=	intel_dsi_panel_vbt.c
#SRCS+=	intel_dsi_pll.c
SRCS+=	intel_dvo.c
SRCS+=	intel_fbc.c
SRCS+=	intel_fbdev.c
SRCS+=	intel_fifo_underrun.c
SRCS+=	intel_frontbuffer.c
SRCS+=	intel_guc_loader.c
SRCS+=	intel_hdmi.c
SRCS+=	intel_hotplug.c
SRCS+=	intel_i2c.c
SRCS+=	intel_lrc.c
SRCS+=	intel_lvds.c
SRCS+=	intel_mocs.c
SRCS+=	intel_modes.c
SRCS+=	intel_opregion.c
SRCS+=	intel_overlay.c
SRCS+=	intel_panel.c
SRCS+=	intel_pm.c
SRCS+=	intel_psr.c
SRCS+=	intel_renderstate_gen6.c
SRCS+=	intel_renderstate_gen7.c
SRCS+=	intel_renderstate_gen8.c
SRCS+=	intel_renderstate_gen9.c
SRCS+=	intel_ringbuffer.c
SRCS+=	intel_runtime_pm.c
SRCS+=	intel_sdvo.c
SRCS+=	intel_sideband.c
SRCS+=	intel_sprite.c
SRCS+=	intel_tv.c
SRCS+=	intel_uncore.c

.include <bsd.kmodule.mk>
