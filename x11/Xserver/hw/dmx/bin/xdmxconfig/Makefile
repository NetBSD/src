# $NetBSD: Makefile,v 1.1 2005/03/22 04:18:49 rtr Exp $

.include <bsd.own.mk>

PROG=	xdmxconfig

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx/config
SRCS=	Canvas.c xdmxconfig.c

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/dmx
SRCS+=	dmxlog.c

_SL=		hw/dmx/dmxconfig
LIBDIR.${_SL}!=	cd ${NETBSDSRCDIR}/x11/Xserver/${_SL} && ${PRINTOBJDIR}

LDADD=	${LIBDIR.hw/dmx/dmxconfig}/lib${_SL:T}.a -lXaw -lXmu -lXt -lSM \
	-lICE -lXpm -lXext -lX11 -lXt -lSM -lICE
DPADD=  ${LIBDIR.hw/dmx/dmxconfig}/lib${_SL:T}.a ${LIBXAW} ${LIBXMU} \
	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11} \
	${LIBXT} ${LIBSM} ${LIBICE}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/dmx \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/include
CPPFLAGS+=	${X11FLAGS.SERVER} -DDMX_LOG_STANDALONE

.include <bsd.x11.mk>
.include <bsd.prog.mk>
