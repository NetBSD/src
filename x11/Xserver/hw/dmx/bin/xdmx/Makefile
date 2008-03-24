# $NetBSD: Makefile,v 1.2.14.1 2008/03/24 07:16:38 keiichi Exp $

NOMAN=# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/examples
PROG=	xdmx

LDADD=	-ldmx -lXext -lX11
DPADD=  ${LIBDMX} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

PROGNAME=dmx

.include <bsd.x11.mk>
.include <bsd.prog.mk>
