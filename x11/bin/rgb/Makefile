#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\" -DUSE_RGB_TXT
CPPFLAGS+=-I${X11SRCDIR.xc}/programs/Xserver/include

FILES=	rgb.txt
FILESDIR=${X11LIBDIR}

.PATH:	${X11SRCDIR.xc}/programs/rgb

.include <bsd.prog.mk>
.include <bsd.x11.mk>
