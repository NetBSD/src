#	$NetBSD: Makefile,v 1.1.2.2 2004/11/21 10:01:28 rtr Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		xfixes

.PATH:		${X11SRCDIR.xc}/programs/Xserver/xfixes
SRCS=		cursor.c region.c saveset.c select.c xfixes.c

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/fixes \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11

.include <bsd.x11.mk>
.include <bsd.lib.mk>
