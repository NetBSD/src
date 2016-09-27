#	$NetBSD: Makefile,v 1.6 2016/09/27 02:33:16 christos Exp $

DRIVER=		xf86-video-vesa
DRIVER_NAME=	vesa_drv

SRCS=		vesa.c
MAN=		vesa.4

COPTS.vesa.c+=	-Wno-error
CPPFLAGS+=-DXFree86LOADER

.include "../Makefile.xf86-driver"
