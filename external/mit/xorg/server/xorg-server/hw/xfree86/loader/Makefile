#	$NetBSD: Makefile,v 1.4 2009/06/11 05:43:17 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	loader
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}

SRCS=	loader.c loadext.c loadmod.c loadfont.c dlloader.c os.c dixsym.c  \
	extsym.c misym.c xf86sym.c 

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SRCS+=	SparcMulDiv.S
.endif

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/parser \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dixmods/extmod \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/vbe \
		-I${X11SRCDIR.xorg-server}/miext/cw \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/int10 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/modes \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ramdac \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/os \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg

CPPFLAGS+=	${X11FLAGS.OS_DEFINES} -DDLOPEN_SUPPORT -DHAS_SHM \
		-DIN_LOADER \
		${x11FLAGS.DIX}

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DUseMMAP
.endif

CPPFLAGS.loadmod.c=	${X11FLAGS.EXTENSION} -DDEFAULT_MODULE_PATH=\"${X11USRLIBDIR}/modules\"
CPPFLAGS.xf86sym.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.dixsym.c=	${X11FLAGS.EXTENSION}

COPTS.dixsym.c=		-Wno-error
COPTS.misym.c=		-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
