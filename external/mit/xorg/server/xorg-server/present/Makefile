#	$NetBSD: Makefile,v 1.1 2016/08/12 01:15:51 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            present

.PATH:          ${X11SRCDIR.xorg-server}/present
SRCS=           present.c \
		present_event.c \
		present_fake.c \
		present_fence.c \
		present_notify.c \
		present_request.c \
		present_screen.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
