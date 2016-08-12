#	$NetBSD: Makefile,v 1.1 2016/08/12 01:15:51 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            pseudoramiX

.PATH:          ${X11SRCDIR.xorg-server}/pseudoramiX
SRCS=           pseudoramiX.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
