#	$NetBSD: Makefile,v 1.5 2016/08/14 03:43:05 mrg Exp $

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
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/Xi \
		-I${X11SRCDIR.xorg-server}/xfixes \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/present \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

# XXX
COPTS.cursor.c+=	-Wno-error
COPTS.region.c+=	-Wno-error
COPTS.select.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
