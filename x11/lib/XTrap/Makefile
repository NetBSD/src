#	$NetBSD: Makefile,v 1.6 2004/08/05 01:28:48 lukem Exp $

.include <bsd.own.mk>

LIB=	XTrap
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XECallBcks.c XEConTxt.c XEDsptch.c XEPrInfo.c XERqsts.c \
	XEStrMap.c XETrapInit.c XEWrappers.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

CPPFLAGS.XEWrappers.c=	-I${X11SRCDIR.xc}/lib/Xt -I${DESTDIR}${X11INCDIR}/X11

LIBDPLIBS=\
	Xt	${.CURDIR}/../Xt \
	SM	${.CURDIR}/../SM \
	ICE	${.CURDIR}/../ICE \
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
