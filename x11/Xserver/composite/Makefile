#	$NetBSD: Makefile,v 1.1.2.2 2004/11/21 10:01:26 rtr Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		composite
.PATH:		${X11SRCDIR.xc}/programs/Xserver/composite

SRCS=		compalloc.c compext.c compinit.c compwindow.c

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/xfixes \
		-I${X11SRCDIR.xc}/programs/Xserver/damageext \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/damage \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/include/extensions \
		-I${DESTDIR}${X11INCDIR}/X11

.include <bsd.x11.mk>
.include <bsd.lib.mk>
