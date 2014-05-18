# $NetBSD: Makefile,v 1.2.4.2 2014/05/18 17:46:11 rmind Exp $

.include "../Makefile.inc"

.PATH: ${S}/external/bsd/drm2/linux

CPPFLAGS+=	-I${S}/external/bsd/drm2/include

# XXX Kludge!
CPPFLAGS+=	-DDIAGNOSTIC

KMOD=	drmkms_linux

SRCS+=	linux_gfp.c
SRCS+=	linux_kmap.c
SRCS+=	linux_i2c.c
SRCS+=	linux_idr.c
SRCS+=	linux_list_sort.c
SRCS+=	linux_module.c
SRCS+=	linux_work.c

.include <bsd.kmodule.mk>
