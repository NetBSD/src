# $NetBSD: Makefile,v 1.2 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=	xf24_32bpp
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	cfb24_32module.c cfbcpyarea.c cfbgc24.c cfbgc32.c cfbgcmisc.c \
	cfbimage.c cfbpixmap.c cfbscrinit.c cfbwindow.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

CPPFLAGS.cfbgc32.c=	-DPSZ=32
CPPFLAGS.cfbgc24.c=	-DPSZ=24

BUILDSYMLINKS=	cfbgc.c cfbgc24.c \
		cfbgc.c cfbgc32.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
