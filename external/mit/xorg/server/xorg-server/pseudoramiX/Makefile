#	$NetBSD: Makefile,v 1.2.2.2 2016/09/14 03:04:16 pgoyette Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=            pseudoramiX

.PATH:          ${X11SRCDIR.xorg-server}/pseudoramiX
SRCS=           pseudoramiX.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
