#	$NetBSD: Makefile,v 1.1.14.1 2010/04/21 05:24:43 matt Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		damageext
.PATH:		${X11SRCDIR.xorg-server}/damageext

SRCS=		damageext.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/xfixes \
		-I${X11SRCDIR.xorg-server}/damagext \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

COPTS.damageext.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
