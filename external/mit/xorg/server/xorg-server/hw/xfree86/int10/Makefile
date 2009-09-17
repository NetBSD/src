# $NetBSD: Makefile,v 1.3.8.1 2009/09/17 04:24:40 snj Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=		int10

.PATH:          ${X11SRCDIR.xorg-server}/hw/xfree86/int10
SRCS=		x86emu.c generic.c xf86x86emu.c xf86int10.c helper_mem.c \
		helper_exec.c xf86int10module.c

CPPFLAGS+=	-D__DRIVER__ -DFORCE_POST -D_CEXPORT= -DNO_LONG_LONG \
		-D_X86EMU -DNO_SYS_HEADERS

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-D_PC
.endif

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/x86emu \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
