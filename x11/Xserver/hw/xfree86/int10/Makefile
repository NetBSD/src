# $NetBSD: Makefile,v 1.3 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=		int10

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}
.PATH:	${X11SRCDIR.xc}/extras/x86emu/src/x86emu
SRCS=	pci.c xf86int10module.c helper_exec.c helper_mem.c xf86int10.c \
	xf86x86emu.c generic.c debug.c decode.c fpu.c ops.c ops2.c \
	prim_ops.c sys.c

CPPFLAGS+=	-D__DRIVER__ -DFORCE_POST -D_CEXPORT= -DNO_LONG_LONG \
		-D_X86EMU -D_PC

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
