#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:13 mrg Exp $

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
