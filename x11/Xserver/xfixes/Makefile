#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:45 rtr Exp $

.include "../Makefile.serverlib"

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
