#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:07 lukem Exp $

.include "../../../Makefile.serverlib"

LIB=	xloader
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader

SRCS=	aoutloader.c coffloader.c dlloader.c elfloader.c hash.c loader.c \
	loadmod.c loadfont.c loadext.c os.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/dbe \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/font/fontcache \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} -DDLOPEN_SUPPORT \
		-DHAS_SHM -DFONTENC_COMPATIBILITY -DXFree86LOADER

CPPFLAGS.loadmod.c=	${X11FLAGS.EXTENSION} \
			-DDEFAULT_MODULE_PATH=\"${X11USRLIBDIR}/modules\"

COPTS.coffloader.c=	-Wno-error
COPTS.elfloader.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
