#	$NetBSD: Makefile,v 1.5 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	XTrap
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XECallBcks.c XEConTxt.c XEDsptch.c XEPrInfo.c XERqsts.c \
	XEStrMap.c XETrapInit.c XEWrappers.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

CPPFLAGS.XEWrappers.c=	-I${X11SRCDIR.xc}/lib/Xt -I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	Xt ../Xt \
	SM ../SM \
	ICE ../ICE \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
