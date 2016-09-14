#	$NetBSD: Makefile,v 1.2.2.2 2016/09/14 03:04:16 pgoyette Exp $

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
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/miext/sync \
		-I${X11SRCDIR.xorg-server}/xfixes \
		-I${X11SRCDIR.xorg-server}/Xi \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
