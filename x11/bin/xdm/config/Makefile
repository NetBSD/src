#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:16 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	Xresources Xservers.ws xdm-config 

CPPSCRIPTFLAGS_Xresources=	-DXPM -DBITMAPDIR=${X11LIBDIR}/xdm/pixmaps \
				-DXDM_PIXMAP=XFree86.xpm \
				-DXDM_BWPIXMAP=XFree86bw.xpm
CPPSCRIPTFLAGS_Xservers.ws=	-DBINDIR=${X11BINDIR} -DDEFAULTVT=vt05
CPPSCRIPTFLAGS_xdm-config=	-DXDMDIR=${X11LIBDIR}/xdm -DXDMLOGDIR=/var/log \
				-DXDMPIDDIR=/var/run -DSU="su -m"

FILES=			GiveConsole TakeConsole XFree86.xpm XFree86bw.xpm \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling \
			${CPPSCRIPTS} 
FILESDIR=		${X11LIBDIR}/xdm
FILESDIR_XFree86.xpm=	${X11LIBDIR}/xdm/pixmaps
FILESDIR_XFree86bw.xpm=	${X11LIBDIR}/xdm/pixmaps

SYMLINKS=	Xservers.ws ${X11LIBDIR}/xdm/Xservers

LNFILES=	Xres.cpp Xresources.cpp \
		Xserv.ws.cpp Xservers.ws.cpp \
		xdm-conf.cpp xdm-config.cpp

.PATH:		${X11SRCDIR.xc}/programs/xdm/config

.include <bsd.x11.mk>
.include <bsd.prog.mk>
