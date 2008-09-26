#	$NetBSD: Makefile,v 1.4 2008/09/26 06:57:03 rtr Exp $

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

CPPFLAGS+=	-DXFree86LOADER

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
