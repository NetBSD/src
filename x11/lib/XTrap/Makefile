#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:11 lukem Exp $

.include <bsd.own.mk>

LIB=	XTrap
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XECallBcks.c XEConTxt.c XEDsptch.c XEPrInfo.c XERqsts.c \
	XEStrMap.c XETrapInit.c XEWrappers.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

CPPFLAGS.XEWrappers.c=	-I${X11SRCDIR.xc}/lib/Xt -I${DESTDIR}/${X11INCDIR}/X11

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
