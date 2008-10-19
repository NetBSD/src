#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:22 haad Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		render

.PATH:		${X11SRCDIR.xorg-server}/render
SRCS=		animcur.c filter.c glyph.c miglyph.c miindex.c mipict.c \
		mirect.c mitrap.c mitri.c picture.c render.c renderedge.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

COPTS.miglyph.c=-Wno-error	# XXX gcc 3.3
COPTS.mirect.c=	-Wno-error	# XXX gcc 3.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
