#	$NetBSD: Makefile,v 1.5.2.2.2.2 2005/03/05 17:40:54 he Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	Xresources Xservers.ws xdm-config 

CPPSCRIPTFLAGS_Xresources=	-DXPM -DBITMAPDIR=${X11LIBDIR}/xdm/pixmaps \
				-DXDM_PIXMAP=NetBSD.xpm \
				-DXDM_BWPIXMAP=NetBSDbw.xpm

# native only gives DEFAULTVT for i386 and amd64.
.if ${MACHINE} == "i386" || ${MACHINE} == "amd64" || ${MACHINE} == "cats"
DEFAULTVT=vt05
.else
DEFAULTVT=	# defined empty
.endif

CPPSCRIPTFLAGS_Xservers.ws=	-DBINDIR=${X11BINDIR} -DDEFAULTVT=${DEFAULTVT}
CPPSCRIPTFLAGS_xdm-config=	-DXDMDIR=${X11LIBDIR}/xdm -DXDMLOGDIR=/var/log \
				-DXDMAUTHDIR=${X11LIBDIR}/xdm \
				-DXDMPIDDIR=/var/run -DSU="su -m"

FILES=			GiveConsole NetBSD.xpm NetBSDbw.xpm NetBSDinv.xpm \
			TakeConsole XFree86.xpm XFree86bw.xpm \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling \
			${CPPSCRIPTS} 
FILESDIR=		${X11LIBDIR}/xdm
FILESDIR_NetBSD.xpm=	${X11LIBDIR}/xdm/pixmaps
FILESDIR_NetBSDbw.xpm=	${X11LIBDIR}/xdm/pixmaps
FILESDIR_NetBSDinv.xpm=	${X11LIBDIR}/xdm/pixmaps
FILESDIR_XFree86.xpm=	${X11LIBDIR}/xdm/pixmaps
FILESDIR_XFree86bw.xpm=	${X11LIBDIR}/xdm/pixmaps

SYMLINKS=	Xservers.ws ${X11LIBDIR}/xdm/Xservers

BUILDSYMLINKS=	Xres.cpp Xresources.cpp \
		Xserv.ws.cpp Xservers.ws.cpp \
		xdm-conf.cpp xdm-config.cpp

.PATH:		${X11SRCDIR.xc}/programs/xdm/config

.include "../Makefile.xdm"

CPPSCRIPTFLAGS_xdm-config+=	-DCHOOSERPATH=${CHOOSERPATH}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
