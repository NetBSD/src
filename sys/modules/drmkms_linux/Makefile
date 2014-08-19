# $NetBSD: Makefile,v 1.4.4.2 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH: ${S}/external/bsd/drm2/linux

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

.include <bsd.kmodule.mk>
