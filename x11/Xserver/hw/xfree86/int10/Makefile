# $NetBSD: Makefile,v 1.4.2.1 2004/11/15 09:12:42 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=		int10

.PATH:          ${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10
SRCS=		x86emu.c generic.c xf86x86emu.c xf86int10.c helper_mem.c \
		helper_exec.c xf86int10module.c pci.c

CPPFLAGS+=	-D__DRIVER__ -DFORCE_POST -D_CEXPORT= -DNO_LONG_LONG \
		-D_X86EMU -DNO_SYS_HEADERS \
		-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-D_PC
.endif

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/extras/x86emu/include \
		-I${X11SRCDIR.xc}/extras/x86emu/src/x86emu \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
