#	$NetBSD: Makefile,v 1.1 2003/12/22 13:13:53 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	i2c
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	xf86i2c.c

.if defined(X11FLAGS.LOADABLE)
SRCS+=		xf86i2cmodule.c
.endif

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
