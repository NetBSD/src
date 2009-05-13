#	$NetBSD: Makefile,v 1.5.18.1 2009/05/13 19:20:48 jym Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	sunffb_drv
MAN=	sunffb.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunffb

SRCS=	ffb_driver.c ffb_accel.c ffb_attr.c ffb_stip.c ffb_bcopy.c \
	ffb_cplane.c ffb_fspans.c ffb_point.c ffb_seg.c ffb_plygon.c \
	ffb_line.c ffb_glyph.c ffb_frect.c ffb_circle.c ffb_checks.c \
	ffb_stubs.c ffb_gc.c ffb_gspans.c ffb_sspans.c ffb_zeroarc.c \
	ffb_cursor.c ffb_dga.c ffb_dac.c ffb_wid.c ffb_dbe.c

SRCS+=	VISmoveImage.s ffb_asm.s

.if ${MACHINE_ARCH} == "sparc64"
AFLAGS+= -x assembler-with-cpp -Wa,-Av9a
.endif

.if ${MACHINE_ARCH} == "sparc"
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


.if ${X11DRI} != "no"
SRCS+=		ffb_dri.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/include

CPPFLAGS+=	${X11FLAGS.DRI}
.endif

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
