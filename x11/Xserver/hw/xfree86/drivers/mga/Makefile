#	$NetBSD: Makefile,v 1.5 2004/03/06 03:31:06 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	mga_drv
MAN=	mga.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/mga

SRCS=		mga_driver.c mga_hwcurs.c mga_dac3026.c mga_dacG.c \
		mga_storm8.c mga_storm16.c mga_storm24.c mga_storm32.c \
		mga_arc.c mga_dga.c mga_shadow.c mga_video.c \
		mga_g450pll.c mga_dh.c mga_esc.c mga_merge.c

BUILDSYMLINKS=	mga_storm.c mga_storm8.c \
		mga_storm.c mga_storm16.c \
		mga_storm.c mga_storm24.c \
		mga_storm.c mga_storm32.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/mga \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/mga/HALlib \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/fbdevhw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf8_32bpp \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf24_32bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
		-I${X11SRCDIR.xc}/include/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc} \
                -I${XSERVERINCDIR}

.if ${X11DRI} != "no"
SRCS+=		mga_dri.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/include

CPPFLAGS+=	${X11FLAGS.DRI}
.endif

CPPFLAGS.mga_storm8.c=	-DPSZ=8
CPPFLAGS.mga_storm16.c=	-DPSZ=16
CPPFLAGS.mga_storm24.c=	-DPSZ=24
CPPFLAGS.mga_storm32.c=	-DPSZ=32

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
