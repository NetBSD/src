#	$NetBSD: Makefile,v 1.8 2022/05/03 13:46:22 uwe Exp $

.include <bsd.own.mk>

LIB=	Xcursor

.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/Xcursor
.PATH:	${X11SRCDIR.${LIB}}/man
SRCS=	cursor.c display.c file.c library.c xlib.c

INCS=	Xcursor.h
INCSDIR=${X11INCDIR}/X11/Xcursor

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DICONDIR=\"${X11LIBDIR}/icons\"
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/Xcursor
CPPFLAGS+=	-I${X11SRCDIR.${LIB}}/../include

LIBDPLIBS=\
	Xrender	${.CURDIR}/../libXrender \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}


MAN = Xcursor.3

X11EXTRAMANDEFS += -e s%__XCURSORPATH__%${__XCURSORPATH__:Q}%g

__XCURSORPATH__	= \
	~/.local/share/icons, \
	~/.icons, \
	/usr/share/icons, \
	/usr/share/pixmaps, \
	${X11LIBDIR}/icons

MLINKS += Xcursor.3 XcursorCursorsCreate.3
MLINKS += Xcursor.3 XcursorCursorsDestroy.3
MLINKS += Xcursor.3 XcursorFilenameLoad.3
MLINKS += Xcursor.3 XcursorFilenameLoadAllImages.3
MLINKS += Xcursor.3 XcursorFilenameLoadCursor.3
MLINKS += Xcursor.3 XcursorFilenameLoadImage.3
MLINKS += Xcursor.3 XcursorFilenameLoadImages.3
MLINKS += Xcursor.3 XcursorFilenameSave.3
MLINKS += Xcursor.3 XcursorFilenameSaveImages.3
MLINKS += Xcursor.3 XcursorGetDefaultSize.3
MLINKS += Xcursor.3 XcursorGetTheme.3
MLINKS += Xcursor.3 XcursorImageCreate.3
MLINKS += Xcursor.3 XcursorImageDestroy.3
MLINKS += Xcursor.3 XcursorImagesCreate.3
MLINKS += Xcursor.3 XcursorImagesDestroy.3
MLINKS += Xcursor.3 XcursorLibraryLoadCursor.3
MLINKS += Xcursor.3 XcursorLibraryLoadCursors.3
MLINKS += Xcursor.3 XcursorLibraryLoadImage.3
MLINKS += Xcursor.3 XcursorLibraryLoadImages.3
MLINKS += Xcursor.3 XcursorSetDefaultSize.3
MLINKS += Xcursor.3 XcursorSetTheme.3
MLINKS += Xcursor.3 XcursorShapeLoadCursor.3
MLINKS += Xcursor.3 XcursorShapeLoadCursors.3
MLINKS += Xcursor.3 XcursorShapeLoadImage.3
MLINKS += Xcursor.3 XcursorShapeLoadImages.3
MLINKS += Xcursor.3 XcursorSupportsARGB.3
MLINKS += Xcursor.3 XcursorXcFileLoad.3
MLINKS += Xcursor.3 XcursorXcFileLoadAllImages.3
MLINKS += Xcursor.3 XcursorXcFileLoadImage.3
MLINKS += Xcursor.3 XcursorXcFileLoadImages.3
MLINKS += Xcursor.3 XcursorXcFileSave.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
