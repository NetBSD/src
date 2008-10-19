#	$NetBSD: Makefile,v 1.5.2.2 2008/10/19 22:41:18 haad Exp $

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
	-I${X11SRCDIR.xorg-server}/hw/xfree86/loader

CPPFLAGS+= \
	${X11FLAGS.EXTENSION} \
	-DHAVE_DIX_CONFIG_H \
	-DXFree86LOADER

SUBDIR = \
	GLcore \
	afb \
	cfb \
	cfb32 \
	dbe \
	extmod \
	fb \
	freetype \
	glx \
	mfb \
	record \
	shadow \
	type1 \
	wfb \
	xorgxkb \
	xtrap

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
