#	$NetBSD: Makefile,v 1.8 2005/09/24 13:51:46 chs Exp $

.include <bsd.own.mk>

LIB=	XTrap
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XECallBcks.c XEConTxt.c XEDsptch.c XEPrInfo.c XERqsts.c \
	XEStrMap.c XETrapInit.c XEWrappers.c

CPPFLAGS+=	${X11FLAGS.THREADS}

CPPFLAGS.XEWrappers.c=	-I${X11SRCDIR.xc}/lib/Xt -I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	Xt	${.CURDIR}/../Xt \
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
