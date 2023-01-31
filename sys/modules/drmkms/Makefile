# $NetBSD: Makefile,v 1.19 2023/01/31 13:09:04 riastradh Exp $

.include "../Makefile.inc"
.include "Makefile.inc"

KMOD=	drmkms
MKLDSCRIPT=yes

.PATH:	${S}/external/bsd/drm2/drm
.PATH:	${S}/external/bsd/drm2/i2c
.PATH:	${S}/external/bsd/drm2/pci

.PATH:	${S}/external/bsd/drm2/dist/drm
.PATH:	${S}/external/bsd/drm2/dist/drm/scheduler

# NetBSD additions.
SRCS+=	drm_agp_hook.c
SRCS+=	drm_cdevsw.c
SRCS+=	drm_gem_vm.c
SRCS+=	drm_module.c
SRCS+=	drm_sysctl.c

# pci bus ids -- doesn't depend on all of pci, just the header files
SRCS+=	drm_pci_busid.c

# Generic, unaccelerated kms framebuffer.
SRCS+=	drmfb.c

# XXX ttm

CPPFLAGS+=	-I.

# XXX CWARNFLAGS.foo.c doesn't work.
COPTS.drm_atomic_helper.c+=	-Wno-shadow
COPTS.drm_bufs.c+=		-Wno-pointer-arith
COPTS.drm_crtc.c+=		-Wno-missing-field-initializers
COPTS.drm_crtc.c+=		-Wno-pointer-arith
COPTS.drm_crtc.c+=		-Wno-shadow
COPTS.drm_client_modeset.c+=	-Wno-shadow
COPTS.drm_dp_helper.c+=		-Wno-pointer-arith
COPTS.drm_fb_helper.c+=		-Wno-shadow
COPTS.drm_edid.c+=		-Wno-shadow
COPTS.drm_ioctl.c+=		-Wno-shadow
COPTS.drm_mm.c+=		${${ACTIVE_CC} == clang :? -Wno-unused-function :}
COPTS.drm_modes.c+=		-Wno-shadow

# Legacy user-mode setting
SRCS+=	drm_bufs.c
SRCS+=	drm_context.c
SRCS+=	drm_dma.c
SRCS+=	drm_legacy_misc.c
SRCS+=	drm_lock.c
SRCS+=	drm_scatter.c

# GPU scheduler is separatd out into drmkms_sched.kmod

# Upstream source files.
#SRCS+=	drm_agpsupport.c
SRCS+=	drm_atomic.c
SRCS+=	drm_atomic_helper.c
SRCS+=	drm_atomic_state_helper.c
SRCS+=	drm_atomic_uapi.c
SRCS+=	drm_auth.c
SRCS+=	drm_blend.c
SRCS+=	drm_bridge.c
SRCS+=	drm_cache.c
SRCS+=	drm_client.c
SRCS+=	drm_client_modeset.c
SRCS+=	drm_color_mgmt.c
SRCS+=	drm_connector.c
SRCS+=	drm_crtc.c
SRCS+=	drm_crtc_helper.c
SRCS+=	drm_damage_helper.c
SRCS+=	drm_dp_dual_mode_helper.c
SRCS+=	drm_dp_helper.c
SRCS+=	drm_dp_mst_topology.c
SRCS+=	drm_drv.c
SRCS+=	drm_dsc.c
SRCS+=	drm_dumb_buffers.c
SRCS+=	drm_edid.c
SRCS+=	drm_encoder.c
SRCS+=	drm_encoder_slave.c
SRCS+=	drm_fb_helper.c
SRCS+=	drm_file.c
SRCS+=	drm_flip_work.c
#SRCS+=	drm_format_helper.c
SRCS+=	drm_fourcc.c
SRCS+=	drm_framebuffer.c
SRCS+=	drm_gem.c
SRCS+=	drm_gem_cma_helper.c
SRCS+=	drm_gem_framebuffer_helper.c
SRCS+=	drm_hashtab.c
SRCS+=	drm_hdcp.c
SRCS+=	drm_ioctl.c
SRCS+=	drm_irq.c
#SRCS+=	drm_kms_helper_common.c
SRCS+=	drm_lease.c
SRCS+=	drm_memory.c
SRCS+=	drm_mm.c
SRCS+=	drm_mode_config.c
SRCS+=	drm_mode_object.c
SRCS+=	drm_modes.c
SRCS+=	drm_modeset_helper.c
SRCS+=	drm_modeset_lock.c
SRCS+=	drm_panel.c
SRCS+=	drm_panel_orientation_quirks.c
#SRCS+=	drm_pci.c
SRCS+=	drm_plane.c
SRCS+=	drm_plane_helper.c
SRCS+=	drm_prime.c
SRCS+=	drm_print.c
SRCS+=	drm_probe_helper.c
SRCS+=	drm_property.c
SRCS+=	drm_rect.c
SRCS+=	drm_scdc_helper.c
SRCS+=	drm_self_refresh_helper.c
#SRCS+=	drm_simple_kms_helper.c
SRCS+=	drm_syncobj.c
SRCS+=	drm_sysfs.c
SRCS+=	drm_trace_points.c
SRCS+=	drm_vblank.c
SRCS+=	drm_vm.c
SRCS+=	drm_vma_manager.c
SRCS+=	drm_writeback.c

WARNS=	3

.include <bsd.kmodule.mk>
