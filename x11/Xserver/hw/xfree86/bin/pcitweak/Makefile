#	$NetBSD: Makefile,v 1.2.40.1 2009/05/13 19:20:47 jym Exp $

.include <bsd.own.mk>

PROG=	pcitweak
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc

SRCS=	pcitweak.c

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
	-L${DUMMYOBJDIR} -ldummy

DPADD=	${LIBXF86_OS} ${LIBDUMMY}

.if ${MACHINE_ARCH} == "i386"
LDADD+=	-li386
DPADD+=	${LIBI386}
.endif

.if ${MACHINE_ARCH} == "x86_64"
LDADD+=	-lx86_64
DPADD+=	${LIBX86_64}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
