#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:31 lukem Exp $

#XTODO	XftConfig-OBSOLETE in etc/X11/

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
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}/${X11INCDIR}/freetype2

FILES=		XftConfig-OBSOLETE
FILESDIR=	${X11LIBDIR}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
