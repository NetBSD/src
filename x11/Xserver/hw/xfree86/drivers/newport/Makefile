#	$NetBSD: Makefile,v 1.2 2004/01/08 04:20:00 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

.include <bsd.own.mk>

MODULE=	newport_drv	
MAN=	newport.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/newport

SRCS=	newport_driver.c newport_regs.c newport_cmap.c newport_shadow.c \
	newport_cursor.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/newport \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

FILES=			XF86Config
FILESNAME_XF86Config=	XF86Config.indy
FILESDIR=		${X11LIBDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
