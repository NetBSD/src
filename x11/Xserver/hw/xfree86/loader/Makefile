#	$NetBSD: Makefile,v 1.4 2004/04/15 14:52:03 drochner Exp $

.include "../../../Makefile.serverlib"

LIB=	loader
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	aoutloader.c coffloader.c dlloader.c elfloader.c hash.c loader.c \
	loadmod.c loadfont.c loadext.c os.c dixsym.c misym.c xf86sym.c \
	fontsym.c extsym.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
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

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} -DDLOPEN_SUPPORT -DHAS_SHM \
		-DFONTENC_COMPATIBILITY -DXFree86LOADER -DFONTCACHE

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DUseMMAP
.endif

CPPFLAGS.loadmod.c=	${X11FLAGS.EXTENSION} -DDEFAULT_MODULE_PATH=\"${X11USRLIBDIR}/modules\"
CPPFLAGS.xf86sym.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.dixsym.c=	${X11FLAGS.EXTENSION}

COPTS.coffloader.c=	-Wno-error
COPTS.elfloader.c=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
