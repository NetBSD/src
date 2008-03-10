# $NetBSD: Makefile,v 1.2 2008/03/10 01:46:32 dholland Exp $

NOMAN=# defined (must come before bsd.own.mk)

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/examples
PROG=	xinput

LDADD=	-ldmx -lXi -lXext -lX11
DPADD=  ${LIBDMX} ${LIBXI} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
