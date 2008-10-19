#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:28 haad Exp $

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
