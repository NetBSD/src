#	$NetBSD: Makefile,v 1.5.2.1 2004/04/29 06:08:33 jmc Exp $

NOCHECKVER=	yes	# we want to install an older version ...
MKLINKLIB=	no

.include <bsd.own.mk>

LIB=	Xft
.PATH:	${X11SRCDIR.xc}/lib/Xft1

SRCS=	xftcfg.c xftcolor.c xftcore.c xftdbg.c xftdir.c xftdpy.c \
	xftdraw.c xftextent.c xftfont.c xftfreetype.c xftfs.c \
	xftglyphs.c xftinit.c xftlist.c xftmatch.c xftmatrix.c \
	xftname.c xftpat.c xftrender.c xftstr.c xftxlfd.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 -DXFREE86_FT2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
