# $NetBSD: Makefile,v 1.1 2005/03/22 04:18:49 rtr Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/config
PROG=	dmxtodmx

_SL=		hw/dmx/dmxconfig
LIBDIR.${_SL}!=	cd ${NETBSDSRCDIR}/x11/Xserver/${_SL} && ${PRINTOBJDIR}

LDADD=	${LIBDIR.hw/dmx/dmxconfig}/lib${_SL:T}.a
DPADD=  ${LIBDIR.hw/dmx/dmxconfig}/lib${_SL:T}.a

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.SERVER}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
