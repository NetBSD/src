# $NetBSD: Makefile,v 1.1.2.2 2005/04/06 21:22:44 tron Exp $

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
