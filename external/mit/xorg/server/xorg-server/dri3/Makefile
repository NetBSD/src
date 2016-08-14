#	$NetBSD: Makefile,v 1.2 2016/08/14 00:03:58 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            dri3

.PATH:          ${X11SRCDIR.xorg-server}/dri3
SRCS=           dri3.c \
		dri3_request.c \
		dri3_screen.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
