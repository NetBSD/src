# $NetBSD: Makefile,v 1.1 2005/03/22 04:18:49 rtr Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/examples
PROG=	xtest
MKMAN=	no

LDADD=	-lXtst -lXext -lX11
DPADD=  ${LIBXTST} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
