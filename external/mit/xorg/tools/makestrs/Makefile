#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:54 wrstuden Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.Xt}/util

HOSTPROG=	makestrs
HOST_CPPFLAGS+=	-I. -I${DESTDIR}${X11INCDIR}

#	Can't use -I${DESTDIR}${X11INCDIR} since pswrap is needed
#	to create some header files.
#
BUILDSYMLINKS=	${X11SRCDIR.Xt}/include/X11 X11
DPSRCS+=	X11

.include <bsd.hostprog.mk>
