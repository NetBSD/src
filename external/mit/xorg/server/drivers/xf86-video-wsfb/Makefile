#	$NetBSD: Makefile,v 1.6 2022/08/20 01:02:44 rin Exp $

DRIVER=		xf86-video-wsfb
DRIVER_NAME=	wsfb_drv

SRCS=		wsfb_driver.c wsfb_cursor.c
MAN=		wsfb.4

CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

CWARNFLAGS.clang+=	-Wno-format

.include "../Makefile.xf86-driver"

# XXX
# DGA support for this driver is broken:
# - dga(1) crashes X server since pScrn->AdjustFrame is NULL.
# - If nop is provided as pScrn->AdjustFrame, dga(1) crashes X server *AND*
#   kernel. This is because DGA uses physical address of framebuffer via
#   /dev/mem (oops!), but we provide only mmap(2)'ed virtual address.
CPPFLAGS+=	-UXFreeXDGA
