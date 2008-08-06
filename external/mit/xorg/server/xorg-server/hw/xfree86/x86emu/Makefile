# $NetBSD: Makefile,v 1.1 2008/08/06 07:16:41 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	xf86emu

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/x86emu
SRCS=	debug.c \
	decode.c \
	fpu.c \
	ops2.c \
	ops.c \
	prim_ops.c \
	sys.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/x86emu \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${XSERVERINCDIR}

CPPFLAGS+=	${X11FLAGS.DIX}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
