#	$NetBSD: Makefile,v 1.1 2008/08/02 21:19:21 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		xfixes

.PATH:		${X11SRCDIR.xorg-server}/xfixes
SRCS=		cursor.c region.c saveset.c select.c xfixes.c

CPPFLAGS+=	${X11FLAGS.PERVASIVE_EXTENSION} ${X11FLAGS.DIX}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/fixes \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

.include <bsd.x11.mk>
.include <bsd.lib.mk>
