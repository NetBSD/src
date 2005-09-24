#	$NetBSD: Makefile,v 1.8 2005/09/24 13:51:47 chs Exp $

NOCHECKVER=	yes	# we want to install an older version ...
MKLINKLIB=	no

.include <bsd.own.mk>

LIB=	Xft
.PATH:	${X11SRCDIR.xc}/lib/Xft1

SRCS=	xftcfg.c xftcolor.c xftcore.c xftdbg.c xftdir.c xftdpy.c \
	xftdraw.c xftextent.c xftfont.c xftfreetype.c xftfs.c \
	xftglyphs.c xftinit.c xftlist.c xftmatch.c xftmatrix.c \
	xftname.c xftpat.c xftrender.c xftstr.c xftxlfd.c

LIBDPLIBS=\
	X11		${.CURDIR}/../X11/dynamic \
	freetype	${.CURDIR}/../freetype2 \
	fontconfig	${.CURDIR}/../fontconfig/src \
	Xrender		${.CURDIR}/../Xrender

CPPFLAGS+=	${X11FLAGS.THREADS}
CPPFLAGS+=	-DFREETYPE2 -I${DESTDIR}${X11INCDIR}/freetype2 -DXFREE86_FT2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
