#	$NetBSD: Makefile,v 1.9 2004/01/08 04:22:11 rtr Exp $

SUBDIR=	ddc doc drivers dummy etc fbdevhw \
	i2c input int10 loader pcidata \
	rac ramdac scanpci shadowfb vbe vgahw \
	xaa xf1bpp xf24_32bpp xf4bpp xf86 xf86_os \
	xf86config xf8_16bpp xf8_32bpp xloader \
	.WAIT \
	bin

FILES=			XF86Config XF98Config
FILESNAME_XF86Config=	XF86Config.eg
FILESNAME_XF98Config=	XF86Config.98
FILESDIR=		${X11LIBDIR}

.include <bsd.files.mk>
.include <bsd.subdir.mk>
