#	$NetBSD: Makefile,v 1.2 2004/01/02 06:22:05 rtr Exp $

.include <bsd.own.mk>

PROG=	ioport
MKMAN=	no
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc

SRCS=	ioport.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}
XF86OSOBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/xf86_os && ${PRINTOBJDIR}
DUMMYOBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/dummy && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/dummylib \
		-I${XSERVERINCDIR}

LDADD=	-L${XF86OSOBJDIR} -lxf86_os \
	-L${DUMMYOBJDIR} -ldummy \
	-li386 -lm

DPADD=	${LIBXF86_OS} ${LIBDUMMY} ${LIBI386} ${LIBM}

SYMLINKS=	${X11BINDIR}/ioport ${X11BINDIR}/inw \
		${X11BINDIR}/ioport ${X11BINDIR}/inl \
		${X11BINDIR}/ioport ${X11BINDIR}/inb \
		${X11BINDIR}/ioport ${X11BINDIR}/outb \
		${X11BINDIR}/ioport ${X11BINDIR}/outw \
		${X11BINDIR}/ioport ${X11BINDIR}/outl

.include <bsd.x11.mk>
.include <bsd.prog.mk>
