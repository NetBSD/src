# $NetBSD: Makefile,v 1.1 2005/03/22 04:18:49 rtr Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/examples
PROG=	res
MKMAN=	no

LDADD=	-lXRes -lXext -lX11
DPADD=  ${LIBXRES} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
