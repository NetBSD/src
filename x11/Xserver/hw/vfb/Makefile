#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:54:09 lukem Exp $

.include <bsd.own.mk>

LIB=		vfb
.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/vfb
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xext
.PATH:		${X11SRCDIR.xc}/programs/Xserver/Xi
.PATH:		${X11SRCDIR.xc}/programs/Xserver/mi

SRCS=		InitInput.c InitOutput.c dpmsstubs.c miinitext.c stubs.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/render

CPPFLAGS+=	-DDDXOSINIT -DSERVER_LOCK -DDDXOSFATALERROR -DDDXOSVERRORF \
		-DHAS_SHM -DHAS_MMAP -UXFree86LOADER

CPPFLAGS.InitInput.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.miinitext.c=	${X11FLAGS.EXTENSION} \
			-DNO_HW_ONLY_EXTS -DNO_MODULE_EXTS -UXFree86LOADER
CPPFLAGS.dpmsstubs.c=	${X11FLAGS.EXTENSION}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
