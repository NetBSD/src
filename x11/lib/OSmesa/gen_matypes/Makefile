#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/X86

HOSTPROG=	gen_matypes
HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR} \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src

.include <bsd.hostprog.mk>
