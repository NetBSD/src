#	$NetBSD: Makefile,v 1.1.4.1 2004/11/15 09:12:39 rtr Exp $

.include <bsd.own.mk>

PROG=	gtf	
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc

SRCS=	gtf.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/etc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/dummylib \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-D__XSERVERNAME__='"Xorg"'

LDADD=	-lm
DPADD=	${LIBM}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
