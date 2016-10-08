#	$NetBSD: Makefile,v 1.14 2016/10/08 20:08:57 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	dixmods

.PATH:	${X11SRCDIR.xorg-server}/mi
SRCS=	miinitext.c

CPPFLAGS+= \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/pixman-1 \
	-I${DESTDIR}${X11INCDIR}/xorg \
	-I${X11SRCDIR.xorg-server}/Xext \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/loader

CPPFLAGS+= \
	${X11FLAGS.EXTENSION} \
	-I${X11SRCDIR.xorg-server}/../include ${X11FLAGS.DIX}

SUBDIR = \
	fb \
	glx \
	shadow \
	wfb \
	xorgxkb

COPTS.miinitext.c=	-Wno-error	# XXX xf86bigfstr.h

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
