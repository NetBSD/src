# $NetBSD: Makefile,v 1.3.4.3 2017/12/03 11:38:52 jdolecek Exp $

.include "../Makefile.inc"
.include "Makefile.inc"

.PATH:	${S}/external/bsd/drm2/drm
.PATH:	${S}/external/bsd/drm2/i2c
.PATH:	${S}/external/bsd/drm2/pci
.PATH:	${S}/external/bsd/drm2/dist/drm

MKLDSCRIPT=yes
KMOD=	drmkms

# Upstream source files.
#SRCS+=	ati_pcigart.c		# Moved to drmkms_pci module.
#SRCS+=	drm_agpsupport.c	# Moved to drmkms_pci module.
SRCS+=	drm_auth.c
SRCS+=	drm_buffer.c
SRCS+=	drm_bufs.c
SRCS+=	drm_cache.c
SRCS+=	drm_context.c
SRCS+=	drm_crtc.c
SRCS+=	drm_crtc_helper.c
SRCS+=	drm_dma.c
SRCS+=	drm_dp_helper.c
SRCS+=	drm_drv.c
SRCS+=	drm_edid.c
SRCS+=	drm_encoder_slave.c
SRCS+=	drm_fb_helper.c		# XXX Rewrite for wsconsole.
SRCS+=	drm_flip_work.c
SRCS+=	drm_fops.c
SRCS+=	drm_gem.c
SRCS+=	drm_global.c
SRCS+=	drm_hashtab.c
#SRCS+=	drm_info.c		# XXX Rewrite for sysctl or something.
#SRCS+=	drm_ioc32.c
SRCS+=	drm_ioctl.c
SRCS+=	drm_irq.c
SRCS+=	drm_lock.c
SRCS+=	drm_memory.c
SRCS+=	drm_mm.c
SRCS+=	drm_modes.c
#SRCS+=	drm_pci.c		# Moved to drmkms_pci module.
SRCS+=	drm_plane_helper.c
#SRCS+=	drm_platform.c		# XXX Rewrite per platform.
#SRCS+=	drm_prime.c		# XXX Revisit later.
SRCS+=	drm_probe_helper.c
SRCS+=	drm_rect.c
SRCS+=	drm_scatter.c
SRCS+=	drm_stub.c
SRCS+=	drm_sysctl.c
SRCS+=	drm_sysfs.c
SRCS+=	drm_vm.c
SRCS+=	drm_vma_manager.c

# NetBSD additions.
SRCS+=	drm_gem_vm.c
SRCS+=	drm_module.c
SRCS+=	drmfb.c

COPTS.drm_crtc.c+=	-Wno-shadow
COPTS.drm_crtc.c+=	-Wno-missing-field-initializers
COPTS.drm_edid.c+=	-Wno-shadow

CPPFLAGS+=		-DNACPICA=1

.include <bsd.kmodule.mk>
