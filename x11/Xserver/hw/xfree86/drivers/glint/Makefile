#	$NetBSD: Makefile,v 1.3.24.1 2008/01/09 02:02:53 matt Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	glint_drv
MAN=	glint.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/glint

SRCS=	glint_driver.c pm2_dac.c pm2ramdac.c pm2_accel.c pm_dac.c \
	IBMramdac.c pm_accel.c tx_dac.c tx_accel.c pm2v_dac.c \
	pm2vramdac.c pm2_video.c pm3_dac.c pm3_accel.c pm3_video.c \
	TIramdac.c glint_shadow.c sx_accel.c glint_dga.c

.if ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "macppc"
CPPFLAGS+=	-DGLINT_AVOID_INT10
.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/glint \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf24_32bpp \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf8_32bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf4bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/fbdevhw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DPPC_MMIO_IS_BE -DSPARC_MMIO_IS_BE

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
