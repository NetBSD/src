#	$NetBSD: Makefile,v 1.1 2003/12/26 05:23:15 rtr Exp $

.include <bsd.own.mk>

PROG=	scanpci
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc

SRCS=	scanpci.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}
XF86OSOBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/xf86_os && ${PRINTOBJDIR}
SCANPCIOBJDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/hw/xfree86/scanpci && ${PRINTOBJDIR}
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
	-L${SCANPCIOBJDIR} -lscanpci \
	-L${DUMMYOBJDIR} -ldummy

DPADD=	${LIBXF86_OS} ${LIBSCANPCI} ${LIBDUMMY}

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DUSE_I386_IOPL
LDADD+=		-li386
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
