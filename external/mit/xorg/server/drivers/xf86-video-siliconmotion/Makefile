#	$NetBSD: Makefile,v 1.10 2016/08/21 18:23:58 joerg Exp $

DRIVER=		xf86-video-siliconmotion
DRIVER_NAME=	siliconmotion_drv

SRCS=		smi_501.c smi_accel.c smi_xaa.c smi_exa.c smi_dac.c
SRCS+=		smi_driver.c smi_i2c.c smi_video.c
SRCS+=		smi_crtc.c smilynx_crtc.c smi_output.c 
SRCS+=		smilynx_output.c smilynx_hw.c smilynx.h 
SRCS+=		smi501_crtc.c smi501_output.c
MAN=		siliconmotion.4

CPPFLAGS+=	-DHAVE_XMODES -DHAVE_XEXTPROTO_71

.include "../Makefile.xf86-driver"

.if ${HAVE_GCC:U0} > 48
COPTS.smi_video.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
.endif

XORG_PKG_VER_PROG=	${DRIVER}
.include "../../../xorg-pkg-ver.mk"
