#	$NetBSD: Makefile,v 1.8 2004/05/16 10:25:14 lukem Exp $

.include <bsd.own.mk>

CPPSCRIPTS=	Xresources Xservers.ws xdm-config 

CPPSCRIPTFLAGS_Xresources=	-DXPM -DBITMAPDIR=${X11INCDIR}/X11/pixmaps \
				-DXDM_PIXMAP=XFree86.xpm \
				-DXDM_BWPIXMAP=XFree86bw.xpm

# native only gives DEFAULTVT for i386 and amd64.
.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
DEFAULTVT=vt05
.else
DEFAULTVT=	# defined empty
.endif

CPPSCRIPTFLAGS_Xservers.ws=	-DBINDIR=${X11BINDIR} -DDEFAULTVT=${DEFAULTVT}
CPPSCRIPTFLAGS_xdm-config=	-DXDMDIR=${XDMDIR} \
				-DXDMAUTHDIR=${XDMVARDIR} \
				-DXDMLOGDIR=${XDMLOGDIR} \
				-DXDMPIDDIR=${XDMPIDDIR} \
				-DSU="su -m"

CONFIGFILES=		GiveConsole TakeConsole \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling \
			${CPPSCRIPTS} 
FILES=			XFree86.xpm XFree86bw.xpm 
FILESDIR=		${XDMDIR}
FILESDIR_XFree86.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_XFree86bw.xpm=	${X11INCDIR}/X11/pixmaps

CONFIGSYMLINKS=		Xservers.ws ${XDMDIR}/Xservers

BUILDSYMLINKS=	Xres.cpp Xresources.cpp \
		Xserv.ws.cpp Xservers.ws.cpp \
		xdm-conf.cpp xdm-config.cpp

.PATH:		${X11SRCDIR.xc}/programs/xdm/config

.include "../Makefile.xdm"

CPPSCRIPTFLAGS_xdm-config+=	-DCHOOSERPATH=${XDMCHOOSERPATH}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
