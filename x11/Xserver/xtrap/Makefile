#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

LIB=		xtrap
.PATH:		${X11SRCDIR.xc}/programs/Xserver/XTrap

XTODO=	SUBDIR=	module

SRCS=		xtrapddmi.c xtrapdi.c xtrapdiswp.c xtrapditbl.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext

.include <bsd.lib.mk>
.include <bsd.x11.mk>
