#	$NetBSD: Makefile,v 1.1 2008/08/03 03:37:05 mrg Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xnest

.PATH:		${X11SRCDIR.xorg-server}/hw/xnest
.PATH:		${X11SRCDIR.xorg-server}/Xext
.PATH:		${X11SRCDIR.xorg-server}/Xi
.PATH:		${X11SRCDIR.xorg-server}/mi
.PATH:		${X11SRCDIR.xorg-server}/fb
SRCS=		Args.c Color.c Cursor.c Display.c Events.c Font.c \
		GC.c GCOps.c Handlers.c Init.c Keyboard.c \
		Pixmap.c Pointer.c Screen.c Visual.c Window.c \
		dpmsstubs.c stubs.c miinitext.c \
		fbcmap_mi.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xnest \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xorg-server}/os \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11INCS.DIX}

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} \
		${X11FLAGS.EXTENSION} \
		${X11FLAGS.DIX} \
		-DNO_HW_ONLY_EXTS -UXFree86LOADER -UMITSHM \
		-D__XKBDEFRULES__='"xorg"'

CPPFLAGS.Events.c=	-Wno-error
CPPFLAGS.Screen.c=	-Wno-error
CPPFLAGS.miinitext.c=	-UDPMSExtension

.include <bsd.x11.mk>
.include <bsd.lib.mk>
