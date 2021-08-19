#	$NetBSD: Makefile,v 1.3 2021/08/19 11:03:04 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            dri3

.PATH:          ${X11SRCDIR.xorg-server}/dri3
SRCS=           dri3.c \
		dri3_request.c \
		dri3_screen.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.xorg-server}/../include \
		-I${X11SRCDIR.xorg-server}/Xext
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
