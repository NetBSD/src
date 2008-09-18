#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:44 wrstuden Exp $

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

.include <bsd.x11.mk>
.include <bsd.lib.mk>
