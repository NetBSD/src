#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:12 lukem Exp $

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		xnest
.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/xnest
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xi
.PATH:		${X11SRCDIR.xc}/programs/Xserver/mi

SRCS=		Args.c Color.c Cursor.c Display.c Events.c Font.c \
		GC.c GCOps.c GetTime.c Handlers.c Init.c Keyboard.c \
		Pixmap.c Pointer.c Screen.c TestExt.c Visual.c \
		Window.c miinitext.c stubs.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xnest \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions

CPPFLAGS+=	-DDDXOSINIT -DSERVER_LOCK -DDDXOSFATALERROR -DDDXOSVERRORF \
		${X11FLAGS.EXTENSION} \
		-DNO_HW_ONLY_EXTS -UXFree86LOADER -UMITSHM

CPPFLAGS.miinitext.c=	-UDPMSExtension

.include <bsd.x11.mk>
.include <bsd.lib.mk>
