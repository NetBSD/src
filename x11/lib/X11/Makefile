#	$NetBSD: Makefile,v 1.4 2003/09/14 07:56:16 lukem Exp $

.include <bsd.own.mk>

SUBDIR=		dynamic static

INCS=		XKBlib.h Xcms.h Xlib.h Xlibint.h Xlocale.h Xresource.h \
		Xutil.h cursorfont.h
INCSDIR=	${X11INCDIR}/X11

FILES=		XErrorDB XKeysymDB Xcms.txt
FILESDIR=	${X11LIBDIR}

.PATH:		${X11SRCDIR.xc}/lib/X11

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
