# $NetBSD: Makefile,v 1.9 2018/08/27 13:52:58 riastradh Exp $

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

SRCS+=	linux_dmi.c
SRCS+=	linux_fence.c
SRCS+=	linux_i2c.c
SRCS+=	linux_idr.c
SRCS+=	linux_kmap.c
SRCS+=	linux_list_sort.c
SRCS+=	linux_module.c
SRCS+=	linux_rcu.c
SRCS+=	linux_reservation.c
SRCS+=	linux_work.c		# XXX Move me to linux.kmod.
SRCS+=	linux_writecomb.c
SRCS+=	linux_ww_mutex.c

.include <bsd.kmodule.mk>
