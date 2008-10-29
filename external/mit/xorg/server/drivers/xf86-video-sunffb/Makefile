#	$NetBSD: Makefile,v 1.1 2008/10/29 03:13:26 macallan Exp $

DRIVER=		xf86-video-sunffb
DRIVER_NAME=	sunffb_drv

SRCS=		ffb_accel.c ffb_dac.c ffb_driver.c ffb_wid.c ffb_attr.c \
		ffb_ddc.c ffb_cursor.c ffb_dga.c \
		VISmoveImage.s ffb_asm.s

.if ${MACHINE} == "sparc64"
AFLAGS+= -x assembler-with-cpp -Wa,-Av9a
.endif

.if ${MACHINE} == "sparc"
AFLAGS+= -x assembler-with-cpp -Wa,-Av8plusa
COPTS= -mv8plus
.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunffb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/dbe \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf8_32wid \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR} \
		-DUSE_VIS

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/include

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		sunffb.4

.include "../Makefile.xf86-driver"
