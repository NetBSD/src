# $NetBSD: Makefile,v 1.1.2.28 2013/07/24 02:48:21 riastradh Exp $

.include "../Makefile.inc"
.include "Makefile.inc"

.PATH:	${S}/external/bsd/drm2/drm
.PATH:	${S}/external/bsd/drm2/pci
.PATH:	${S}/external/bsd/drm2/linux
.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drm2
IOCONF=	drm.ioconf

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c	# XXX Move to drm2pci module later.
SRCS+=	drm_auth.c
SRCS+=	drm_buffer.c
SRCS+=	drm_bufs.c
#SRCS+=	drm_cache.c		# XXX Rewrite for uvm.
SRCS+=	drm_context.c
SRCS+=	drm_crtc.c
SRCS+=	drm_crtc_helper.c
SRCS+=	drm_dma.c
#SRCS+=	drm_dp_helper.c		# XXX Rewrite for i2c.
SRCS+=	drm_drv.c
#SRCS+=	drm_edid.c		# XXX Rewrite for NetBSD edid.
#SRCS+=	drm_encoder_slave.c	# XXX Rewrite for i2c.
#SRCS+=	drm_fb_helper.c		# XXX Rewrite for wsconsole.
SRCS+=	drm_fops.c
#SRCS+=	drm_gem.c		# XXX Rewrite for uvm and stuff.
SRCS+=	drm_global.c
SRCS+=	drm_hashtab.c
#SRCS+=	drm_info.c		# XXX Rewrite for sysctl or something.
#SRCS+=	drm_ioc32.c
SRCS+=	drm_ioctl.c
SRCS+=	drm_irq.c
SRCS+=	drm_lock.c
SRCS+=	drm_memory.c
SRCS+=	drm_mm.c
#SRCS+=	drm_modes.c		# XXX Rewrite if necessary.
SRCS+=	drm_pci.c		# XXX Move to drm2pci module later.
#SRCS+=	drm_platform.c		# XXX Rewrite per platform.
#SRCS+=	drm_prime.c		# XXX Revisit later.
#SRCS+=	drm_proc.c		# XXX Rewrite for sysctl.
SRCS+=	drm_scatter.c
SRCS+=	drm_stub.c
#SRCS+=	drm_vm.c		# XXX Rewrite for uvm.

SRCS+=	drm_module.c
SRCS+=	linux_idr.c

.include <bsd.kmodule.mk>
