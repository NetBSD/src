#	$NetBSD: Makefile,v 1.1 2003/12/26 05:23:15 rtr Exp $

.include <bsd.own.mk>

PROG=	kbd_mode
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc

SRCS=	kbd_mode.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/dummylib \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DWSCONS_SUPPORT

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
