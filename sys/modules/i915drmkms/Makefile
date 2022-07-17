# $NetBSD: Makefile,v 1.20 2022/07/17 14:11:40 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

KMOD=	i915drmkms
IOCONF=	i915drmkms.ioconf
MKLDSCRIPT=yes

.PATH:	${S}/external/bsd/drm2/i915drm

.PATH:	${S}/external/bsd/drm2/dist/drm/i915
.PATH:	${S}/external/bsd/drm2/dist/drm/i915/display
.PATH:	${S}/external/bsd/drm2/dist/drm/i915/gem
.PATH:	${S}/external/bsd/drm2/dist/drm/i915/gt
.PATH:	${S}/external/bsd/drm2/dist/drm/i915/gt/uc

CPPFLAGS+=	-I${S}/sys/modules/drmkms

CPPFLAGS+=	-I${S}/external/bsd/drm2/i915drm

CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/drm/i915

# Substitute for _KERNEL_OPT flags.
CPPFLAGS+=	-DNACPICA=1
CPPFLAGS+=	-DNVGA=1

CPPFLAGS+=	-DCONFIG_DRM_FBDEV_EMULATION=1
CPPFLAGS+=	-DCONFIG_DRM_I915_DEBUG=1 # XXX
CPPFLAGS+=	-DCONFIG_DRM_I915_DEBUG_GEM=1 # XXX
CPPFLAGS+=	-DCONFIG_DRM_I915_DEBUG_RUNTIME_PM=0
CPPFLAGS+=	-DCONFIG_DRM_I915_PREEMPT_TIMEOUT=640
CPPFLAGS+=	-DCONFIG_DRM_I915_TIMESLICE_DURATION=1
CPPFLAGS+=	-DCONFIG_DRM_I915_ALPHA_SUPPORT=0
CPPFLAGS+=	-DCONFIG_DRM_I915_FBDEV=1
CPPFLAGS+=	-DCONFIG_DRM_I915_GVT=0
CPPFLAGS+=	-DCONFIG_DRM_I915_PRELIMINARY_HW_SUPPORT=0
CPPFLAGS+=	-DCONFIG_DRM_I915_DEBUG_MMIO=1 # XXX
CPPFLAGS+=	-DCONFIG_DRM_I915_FORCE_PROBE=0
CPPFLAGS+=	-DCONFIG_DRM_I915_SPIN_REQUEST=0
CPPFLAGS+=	-DCONFIG_DRM_I915_SW_FENCE_CHECK_DAG=1 # XXX expensive debug option?
CPPFLAGS+=	-DCONFIG_DRM_I915_HEARTBEAT_INTERVAL=2500 # milliseconds
CPPFLAGS+=	-DCONFIG_DRM_I915_STOP_TIMEOUT=100 # milliseconds
CPPFLAGS+=	-DCONFIG_DRM_I915_PREEMPT_TIMEOUT=640 # milliseconds
CPPFLAGS+=	-DCONFIG_DRM_I915_CAPTURE_ERROR=0
CPPFLAGS+=	-DCONFIG_DRM_I915_SELFTEST=0
CPPFLAGS+=	-DCONFIG_DRM_I915_USERFAULT_AUTOSUSPEND=0
CPPFLAGS+=	-DCONFIG_PM=1
CPPFLAGS+=	-DCONFIG_INTEL_MEI_HDCP=0

# This code comes from the Linux kernel, which assumes signed
# overflow is OK.
COPTS+=		-fwrapv

CWARNFLAGS+=	-Wno-missing-field-initializers
CWARNFLAGS+=	-Wno-pointer-arith
CWARNFLAGS+=	-Wno-shadow

COPTS.i915_pci.c+=	${${ACTIVE_CC} == "gcc":? -Wno-override-init :}
COPTS.i915_pci.c+=	${${ACTIVE_CC} == "clang":? -Wno-initializer-overrides :}
COPTS.i915_sw_fence.c+=	${${ACTIVE_CC} == "clang":? -Wno-unused-function :}
COPTS.intel_sprite.c+=	${${ACTIVE_CC} == "gcc":? -Wno-maybe-uninitialized :}
COPTS.intel_ddi.c+=	${GCC_NO_MAYBE_UNINITIALIZED}
COPTS.intel_ddi.c+=	${GCC_NO_IMPLICIT_FALLTHRU}
COPTS.intel_display.c+=	${GCC_NO_IMPLICIT_FALLTHRU}
COPTS.intel_guc_submission.c+=	${${ACTIVE_CC} == "clang":? -Wno-unused-function :}
COPTS.intel_hdmi.c+=	${${ACTIVE_CC} == "clang":? -Wno-unused-function :}
COPTS.intel_sdvo.c+=	${GCC_NO_IMPLICIT_FALLTHRU}

# NetBSD additions.
SRCS+=	i915_module.c
SRCS+=	i915_pci_autoconf.c
SRCS+=	intelfb.c

# Local overrides.
SRCS+=	i915_gem_userptr.c
SRCS+=	i915_gemfs.c
SRCS+=	i915_switcheroo.c
SRCS+=	i915_sysfs.c
SRCS+=	i915_trace_points.c
SRCS+=	icl_dsi.c
SRCS+=	intel_dsi_dcs_backlight.c
SRCS+=	intel_gtt_subr.c
SRCS+=	intel_guc_log.c
SRCS+=	intel_lpe_audio.c
SRCS+=	intel_region_lmem.c
SRCS+=	vlv_dsi.c

# Upstream sources.
#
# sed -E -e 's,^(#?)file	.*/([^ 	]*).*,\1SRCS+=	\2,g'
SRCS+=	dvo_ch7017.c
SRCS+=	dvo_ch7xxx.c
SRCS+=	dvo_ivch.c
SRCS+=	dvo_ns2501.c
SRCS+=	dvo_sil164.c
SRCS+=	dvo_tfp410.c
#SRCS+=	icl_dsi.c
SRCS+=	intel_acpi.c
SRCS+=	intel_atomic.c
SRCS+=	intel_atomic_plane.c
SRCS+=	intel_audio.c
SRCS+=	intel_bios.c
SRCS+=	intel_bw.c
SRCS+=	intel_cdclk.c
SRCS+=	intel_color.c
SRCS+=	intel_combo_phy.c
SRCS+=	intel_connector.c
SRCS+=	intel_crt.c
SRCS+=	intel_ddi.c
SRCS+=	intel_display.c
SRCS+=	intel_display_power.c
SRCS+=	intel_dp.c
SRCS+=	intel_dp_aux_backlight.c
SRCS+=	intel_dp_link_training.c
SRCS+=	intel_dp_mst.c
SRCS+=	intel_dpio_phy.c
SRCS+=	intel_dpll_mgr.c
SRCS+=	intel_dsb.c
#SRCS+=	intel_dsi.c
#SRCS+=	intel_dsi_dcs_backlight.c
#SRCS+=	intel_dsi_vbt.c
SRCS+=	intel_dvo.c
SRCS+=	intel_fbc.c
SRCS+=	intel_fbdev.c
SRCS+=	intel_fifo_underrun.c
SRCS+=	intel_frontbuffer.c
SRCS+=	intel_gmbus.c
SRCS+=	intel_hdcp.c
SRCS+=	intel_hdmi.c
SRCS+=	intel_hotplug.c
#SRCS+=	intel_lpe_audio.c
SRCS+=	intel_lspcon.c
SRCS+=	intel_lvds.c
SRCS+=	intel_opregion.c
SRCS+=	intel_overlay.c
SRCS+=	intel_panel.c
SRCS+=	intel_psr.c
SRCS+=	intel_quirks.c
SRCS+=	intel_sdvo.c
SRCS+=	intel_sprite.c
SRCS+=	intel_tc.c
SRCS+=	intel_tv.c
SRCS+=	intel_vdsc.c
SRCS+=	intel_vga.c
#SRCS+=	vlv_dsi.c
#SRCS+=	vlv_dsi_pll.c
SRCS+=	i915_gem_busy.c
SRCS+=	i915_gem_clflush.c
#SRCS+=	i915_gem_client_blt.c
SRCS+=	i915_gem_context.c
SRCS+=	i915_gem_dmabuf.c
SRCS+=	i915_gem_domain.c
SRCS+=	i915_gem_execbuffer.c
SRCS+=	i915_gem_fence.c
SRCS+=	i915_gem_internal.c
SRCS+=	i915_gem_lmem.c
SRCS+=	i915_gem_mman.c
SRCS+=	i915_gem_object.c
SRCS+=	i915_gem_object_blt.c
SRCS+=	i915_gem_pages.c
SRCS+=	i915_gem_phys.c
SRCS+=	i915_gem_pm.c
SRCS+=	i915_gem_region.c
SRCS+=	i915_gem_shmem.c
SRCS+=	i915_gem_shrinker.c
SRCS+=	i915_gem_stolen.c
SRCS+=	i915_gem_throttle.c
SRCS+=	i915_gem_tiling.c
#SRCS+=	i915_gem_userptr.c
SRCS+=	i915_gem_wait.c
#SRCS+=	i915_gemfs.c
#SRCS+=	debugfs_engines.c
#SRCS+=	debugfs_gt.c
#SRCS+=	debugfs_gt_pm.c
SRCS+=	gen6_ppgtt.c
SRCS+=	gen6_renderstate.c
SRCS+=	gen7_renderstate.c
SRCS+=	gen8_ppgtt.c
SRCS+=	gen8_renderstate.c
SRCS+=	gen9_renderstate.c
SRCS+=	intel_breadcrumbs.c
SRCS+=	intel_context.c
SRCS+=	intel_engine_cs.c
SRCS+=	intel_engine_heartbeat.c
SRCS+=	intel_engine_pm.c
SRCS+=	intel_engine_pool.c
SRCS+=	intel_engine_user.c
SRCS+=	intel_ggtt.c
SRCS+=	intel_gt.c
SRCS+=	intel_gt_irq.c
SRCS+=	intel_gt_pm.c
SRCS+=	intel_gt_pm_irq.c
SRCS+=	intel_gt_requests.c
SRCS+=	intel_gtt.c
SRCS+=	intel_llc.c
SRCS+=	intel_lrc.c
SRCS+=	intel_mocs.c
SRCS+=	intel_ppgtt.c
SRCS+=	intel_rc6.c
SRCS+=	intel_renderstate.c
SRCS+=	intel_reset.c
SRCS+=	intel_ring.c
SRCS+=	intel_ring_submission.c
SRCS+=	intel_rps.c
SRCS+=	intel_sseu.c
SRCS+=	intel_timeline.c
SRCS+=	intel_workarounds.c
SRCS+=	intel_guc.c
SRCS+=	intel_guc_ads.c
SRCS+=	intel_guc_ct.c
SRCS+=	intel_guc_fw.c
#SRCS+=	intel_guc_log.c
SRCS+=	intel_guc_submission.c
SRCS+=	intel_huc.c
SRCS+=	intel_huc_fw.c
SRCS+=	intel_uc.c
SRCS+=	intel_uc_fw.c
SRCS+=	i915_active.c
SRCS+=	i915_buddy.c
SRCS+=	i915_cmd_parser.c
SRCS+=	i915_drv.c
SRCS+=	i915_gem.c
SRCS+=	i915_gem_evict.c
SRCS+=	i915_gem_fence_reg.c
SRCS+=	i915_gem_gtt.c
SRCS+=	i915_getparam.c
SRCS+=	i915_globals.c
SRCS+=	i915_irq.c
SRCS+=	i915_memcpy.c
#SRCS+=	i915_mm.c
SRCS+=	i915_params.c
SRCS+=	i915_pci.c
SRCS+=	i915_perf.c
SRCS+=	i915_query.c
SRCS+=	i915_request.c
SRCS+=	i915_scatterlist.c
SRCS+=	i915_scheduler.c
SRCS+=	i915_suspend.c
SRCS+=	i915_sw_fence.c
SRCS+=	i915_sw_fence_work.c
#SRCS+=	i915_switcheroo.c
SRCS+=	i915_syncmap.c
#SRCS+=	i915_sysfs.c
#SRCS+=	i915_trace_points.c
SRCS+=	i915_user_extensions.c
SRCS+=	i915_utils.c
SRCS+=	i915_vgpu.c
SRCS+=	i915_vma.c
SRCS+=	intel_csr.c
SRCS+=	intel_device_info.c
SRCS+=	intel_memory_region.c
SRCS+=	intel_pch.c
SRCS+=	intel_pm.c
#SRCS+=	intel_region_lmem.c
SRCS+=	intel_runtime_pm.c
SRCS+=	intel_sideband.c
SRCS+=	intel_uncore.c
SRCS+=	intel_wakeref.c
SRCS+=	intel_wopcm.c
#SRCS+=	i915_oa_bdw.c
#SRCS+=	i915_oa_bxt.c
#SRCS+=	i915_oa_cflgt2.c
#SRCS+=	i915_oa_cflgt3.c
#SRCS+=	i915_oa_chv.c
#SRCS+=	i915_oa_cnl.c
#SRCS+=	i915_oa_glk.c
#SRCS+=	i915_oa_hsw.c
#SRCS+=	i915_oa_icl.c
#SRCS+=	i915_oa_kblgt2.c
#SRCS+=	i915_oa_kblgt3.c
#SRCS+=	i915_oa_sklgt2.c
#SRCS+=	i915_oa_sklgt3.c
#SRCS+=	i915_oa_sklgt4.c
#SRCS+=	i915_oa_tgl.c

WARNS=	3

.include <bsd.kmodule.mk>
