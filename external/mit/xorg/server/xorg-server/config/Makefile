#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:17 haad Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		config
.PATH:		${X11SRCDIR.xorg-server}/config

SRCS=		config.c

CPPFLAGS+=	${X11FLAGS.DIX}
CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include

.include <bsd.x11.mk>
.include <bsd.lib.mk>
