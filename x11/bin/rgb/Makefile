#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:09 lukem Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\" -DUSE_RGB_TXT
CPPFLAGS+=-I${X11SRCDIR.xc}/programs/Xserver/include

FILES=	rgb.txt
FILESDIR=${X11LIBDIR}

.PATH:	${X11SRCDIR.xc}/programs/rgb

.include <bsd.x11.mk>
.include <bsd.prog.mk>
