# $NetBSD: Makefile,v 1.4.2.2 2015/03/06 21:39:11 snj Exp $

.include "../Makefile.inc"

.PATH: ${S}/external/bsd/drm2/linux

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
