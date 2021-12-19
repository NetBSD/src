# $NetBSD: Makefile,v 1.14 2021/12/19 11:54:24 riastradh Exp $

#
# At some point this needs to turn into linux.kmod and a proper home for it
# found.  For now pickup the linux_work.c from the (increasingly poorly
# named) common area.
#

.include "../Makefile.inc"

.PATH: ${S}/external/bsd/common/linux
.PATH: ${S}/external/bsd/drm2/linux

CPPFLAGS+=	-I${S}/external/bsd/common/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/include

# XXX Kludge!
CPPFLAGS+=	-DDIAGNOSTIC

KMOD=	drmkms_linux

# common/linux
SRCS+=	linux_rcu.c
SRCS+=	linux_srcu.c
SRCS+=	linux_tasklet.c
SRCS+=	linux_work.c

# drm2/linux
SRCS+=	linux_atomic64.c
SRCS+=	linux_backlight.c
SRCS+=	linux_dma_buf.c
SRCS+=	linux_dma_fence.c
SRCS+=	linux_dma_fence_array.c
SRCS+=	linux_dma_fence_chain.c
SRCS+=	linux_dma_resv.c
SRCS+=	linux_dmi.c
SRCS+=	linux_firmware.c
SRCS+=	linux_hrtimer.c
SRCS+=	linux_i2c.c
SRCS+=	linux_idr.c
SRCS+=	linux_irq_work.c
SRCS+=	linux_kmap.c
SRCS+=	linux_list_sort.c
SRCS+=	linux_module.c
SRCS+=	linux_notifier.c
SRCS+=	linux_pci.c
SRCS+=	linux_radixtree.c
SRCS+=	linux_rwsem.c
SRCS+=	linux_sg.c
SRCS+=	linux_stop_machine.c
SRCS+=	linux_sync_file.c
SRCS+=	linux_wait_bit.c
SRCS+=	linux_writecomb.c
SRCS+=	linux_ww_mutex.c
SRCS+=	linux_xa.c

WARNS=	3

.include <bsd.kmodule.mk>
