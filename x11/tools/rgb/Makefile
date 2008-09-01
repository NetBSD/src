#	$NetBSD: Makefile,v 1.1 2008/09/01 10:24:15 rtr Exp $

NOMAN=		1

.include <bsd.own.mk>

.PATH:		${X11SRCDIR.xc}/programs/rgb
HOSTPROG=	rgb

HOST_CPPFLAGS+=	-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\"
HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${X11SRCDIR.xc}/programs/Xserver/include

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
