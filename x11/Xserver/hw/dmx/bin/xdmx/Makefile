# $NetBSD: Makefile,v 1.3 2008/03/10 01:46:32 dholland Exp $

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
