#	$NetBSD: Makefile,v 1.6.4.2 2004/11/21 10:01:27 rtr Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xnest

.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/xnest
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xi
.PATH:		${X11SRCDIR.xc}/programs/Xserver/mi
SRCS=		Args.c Color.c Cursor.c Display.c Events.c Font.c \
		GC.c GCOps.c GetTime.c Handlers.c Init.c Keyboard.c \
		Pixmap.c Pointer.c Screen.c TestExt.c Visual.c \
		Window.c miinitext.c stubs.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xnest \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} \
		${X11FLAGS.EXTENSION} \
		-DNO_HW_ONLY_EXTS -UXFree86LOADER -UMITSHM \
		-D__XKBDEFRULES__='"xorg"'

CPPFLAGS.miinitext.c=	-UDPMSExtension

.include <bsd.x11.mk>
.include <bsd.lib.mk>
