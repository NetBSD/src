# $NetBSD: Makefile,v 1.2 2003/12/23 14:28:20 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		int10

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}
.PATH:	${X11SRCDIR.xc}/extras/x86emu/src/x86emu
SRCS=	pci.c xf86int10module.c helper_exec.c helper_mem.c xf86int10.c \
	xf86x86emu.c generic.c debug.c decode.c fpu.c ops.c ops2.c \
	prim_ops.c sys.c

CPPFLAGS+=	-D__DRIVER__ -DFORCE_POST -D_CEXPORT= -DNO_LONG_LONG \
		-D_X86EMU -D_PC

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

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
