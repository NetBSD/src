# $NetBSD: Makefile,v 1.1 2004/01/05 10:14:18 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	xf8_32bpp
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	cfb8_32module.c cfbbstore.c cfbcpyarea.c cfbcpyplane.c cfbgc32.c \
	cfbgc8.c cfbgcmisc.c cfbgcunder.c cfbimage.c cfbpntwin.c \
	cfbscrinit.c cfbwindow.c xf86overlay.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${XSERVERINCDIR}

CPPFLAGS.cfbgc32.c=	-DPSZ=32
CPPFLAGS.cfbgc8.c=	-DPSZ=8

BUILDSYMLINKS=	cfbgc.c cfbgc8.c \
		cfbgc.c cfbgc32.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
