#	$NetBSD: Makefile,v 1.1 2003/12/22 16:18:19 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	shadow
.PATH:	${X11SRCDIR.xc}/programs/Xserver/miext/${LIB}

SRCS=	shadow.c shalloc.c shplanar.c shplanar8.c shpacked.c shrotate.c \
	shrot8pack_90.c shrot8pack_180.c shrot8pack_270.c shrot16pack_90.c \
	shrot16pack_180.c shrot16pack_270.c shrot32pack_90.c \
	shrot32pack_180.c shrot32pack_270.c

.if defined(X11FLAGS.LOADABLE)
SRCS+=	shmodule.c
.endif

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/miext/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
