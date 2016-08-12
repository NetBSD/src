#	$NetBSD: Makefile,v 1.1 2016/08/12 01:15:49 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            dri3

.PATH:          ${X11SRCDIR.xorg-server}/dri3
SRCS=           dri3.c \
		dri3_request.c \
		dri3_screen.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
