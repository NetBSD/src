#	$NetBSD: Makefile,v 1.3.40.1 2009/05/13 19:20:48 jym Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	nsc_drv
MAN=	nsc.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nsc

SRCS=	nsc_driver.c nsc_gx1_driver.c nsc_gx1_dga.c nsc_gx1_accel.c \
	nsc_gx1_cursor.c nsc_gx1_video.c nsc_gx1_shadow.c \
	nsc_gx2_driver.c nsc_gx2_accel.c nsc_gx2_cursor.c nsc_gx2_dga.c \
	nsc_gx2_video.c nsc_gx2_shadow.c durango.c panel.c

.if ${MACHINE_ARCH} == "i386"
SRCS+=	nsc_msr_asm.S
.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nsc \
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
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/fbdevhw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nsc/gfx \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nsc/panel \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DOPT_ACCEL

CPPFLAGS.i2c_acc.c=	-Wno-error	# gcc3/macppc
CPPFLAGS.durango.c=	-Wno-error	# gcc3/macppc
.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
