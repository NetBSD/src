# $NetBSD: Makefile,v 1.1 2003/12/22 13:54:13 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		ddc
.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=		ddcProperty.c edid.c interpret_edid.c interpret_vdif.c \
		print_edid.c print_vdif.c xf86DDC.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
