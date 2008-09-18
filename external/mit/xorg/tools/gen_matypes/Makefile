#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:54 wrstuden Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/x86

HOSTPROG=	gen_matypes
HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi \
		-I${X11SRCDIR.xc}/extras/Mesa/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/main

.include <bsd.hostprog.mk>
