# $NetBSD: Makefile,v 1.1.2.19 2013/07/24 02:34:16 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm2/drm
.PATH:	${S}/external/bsd/drm2/linux
.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drm2

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c
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
SRCS+=	drm_pci.c
#SRCS+=	drm_platform.c		# XXX Rewrite per platform.
SRCS+=	drm_prime.c
#SRCS+=	drm_proc.c		# XXX Rewrite for sysctl.
#SRCS+=	drm_scatter.c		# XXX Rewrite for bus_dma(9).
SRCS+=	drm_stub.c
SRCS+=	drm_vm.c

SRCS+=	linux_idr.c

CPPFLAGS+=	-I${S}/external/bsd/drm2/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/uapi
CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/include
CPPFLAGS+=	-D__KERNEL__

# XXX Is this the right place to set this?
CPPFLAGS+=	-DDIAGNOSTIC

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DMTRR
.endif

.include <bsd.kmodule.mk>
