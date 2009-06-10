#	$NetBSD: Makefile,v 1.3 2009/06/10 06:46:34 mrg Exp $

DRIVER=		xf86-video-siliconmotion
DRIVER_NAME=	siliconmotion_drv

SRCS=		smi_accel.c smi_xaa.c smi_exa.c smi_dac.c
SRCS+=		smi_driver.c smi_i2c.c smi_video.c
MAN=		siliconmotion.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H

.include "../Makefile.xf86-driver"
