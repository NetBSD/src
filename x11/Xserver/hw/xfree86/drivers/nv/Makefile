#	$NetBSD: Makefile,v 1.4 2004/01/05 03:47:10 soren Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

.include <bsd.own.mk>

MODULE=	nv_drv
MAN=	nv.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nv

SRCS=	nv_driver.c nv_dac.c nv_setup.c nv_cursor.c nv_xaa.c nv_dga.c \
	nv_shadow.c riva_hw.c nv_video.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nv \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/fbdevhw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
		-I${X11SRCDIR.xc}/include/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DPSZ=8
		
.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
