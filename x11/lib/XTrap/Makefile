#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:18 lukem Exp $

.include <bsd.own.mk>

LIB=	XTrap
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XECallBcks.c XEConTxt.c XEDsptch.c XEPrInfo.c XERqsts.c \
	XEStrMap.c XETrapInit.c XEWrappers.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

CPPFLAGS.XEWrappers.c=	-I${X11SRCDIR.xc}/lib/Xt -I${DESTDIR}/${X11INCDIR}/X11

#LM#	LDADD+=	-lXt -lSM -lICE -lXext -lX11
#LM#	DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
