#	$NetBSD: Makefile,v 1.2 2004/02/08 21:52:30 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/config/util

HOSTPROG=	makestrs
HOST_CPPFLAGS+=	-I.

#	Can't use -I${DESTDIR}${X11INCDIR} since pswrap is needed
#	to create some header files.
#
BUILDSYMLINKS=	${X11SRCDIR.xc}/include X11
DPSRCS+=	X11

.include <bsd.hostprog.mk>
