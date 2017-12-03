# $NetBSD: Makefile,v 1.4.4.3 2017/12/03 11:38:52 jdolecek Exp $

#
# At some point this needs to turn into linux.kmod and a proper home for it
# found.  For now pickup the linux_work.c from the (increasingly poorly
# named) common area.
#

.include "../Makefile.inc"

.PATH: ${S}/external/bsd/drm2/linux
.PATH: ${S}/external/bsd/common/linux

CPPFLAGS+=	-I${S}/external/bsd/common/include
CPPFLAGS+=	-I${S}/external/bsd/drm2/include

# XXX Kludge!
CPPFLAGS+=	-DDIAGNOSTIC

KMOD=	drmkms_linux

SRCS+=	linux_dmi.c
SRCS+=	linux_kmap.c
SRCS+=	linux_i2c.c
SRCS+=	linux_idr.c
SRCS+=	linux_list_sort.c
SRCS+=	linux_module.c
SRCS+=	linux_work.c
SRCS+=	linux_writecomb.c
SRCS+=	linux_ww_mutex.c

.include <bsd.kmodule.mk>
