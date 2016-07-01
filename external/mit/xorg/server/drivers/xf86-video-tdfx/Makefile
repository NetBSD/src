#	$NetBSD: Makefile,v 1.13 2016/07/01 23:29:17 macallan Exp $

DRIVER=		xf86-video-tdfx
DRIVER_NAME=	tdfx_drv

SRCS=		tdfx_accel.c tdfx_dga.c tdfx_driver.c tdfx_hwcurs.c
SRCS+=		tdfx_io.c tdfx_priv.c tdfx_sli.c tdfx_video.c
SRCS+=		tdfx_dri.c
MAN=		tdfx.4

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DUSE_INT10
.else
CPPFLAGS+=	-DUSE_PCIVGAIO=0
.endif
CPPFLAGS+=	-DHAVE_XAA_H
CPPFLAGS+=	-DTDFXDRI

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm

COPTS.tdfx_driver.c=      -Wno-error	# XXX deprecated

CWARNFLAGS.clang+=	-Wno-format -Wno-pointer-sign

.include "../Makefile.xf86-driver"
