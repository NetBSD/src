# $NetBSD: Makefile,v 1.1.2.2 2005/04/06 21:22:45 tron Exp $

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
