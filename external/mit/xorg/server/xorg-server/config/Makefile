#	$NetBSD: Makefile,v 1.3 2021/12/05 22:59:32 nia Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		config
.PATH:		${X11SRCDIR.xorg-server}/config

SRCS=		config.c
SRCS+=		wscons.c

CPPFLAGS+=	${X11FLAGS.DIX}
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

CPPFLAGS+=	-DCONFIG_WSCONS

.include <bsd.x11.mk>
.include <bsd.lib.mk>
