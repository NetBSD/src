#	$NetBSD: Makefile,v 1.3 2008/11/09 13:02:36 veego Exp $

.include <bsd.own.mk>

# native only gives DEFAULTVT for i386 and amd64.
.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
DEFAULTVT=vt05
.else
DEFAULTVT=	# defined empty
.endif

CPPSCRIPTS=	Xresources Xservers.ws Xsession xdm-config 

CPPSCRIPTFLAGS_Xresources=	-DXPM -DBITMAPDIR=${X11INCDIR}/X11/pixmaps \
				-DXDM_PIXMAP=xorg.xpm \
				-DXDM_BWPIXMAP=xorg-bw.xpm
CPPSCRIPTFLAGS_Xservers.ws=	-DBINDIR=${X11BINDIR} -DDEFAULTVT=${DEFAULTVT}
CPPSCRIPTFLAGS_Xsession=	-DBINDIR=${X11BINDIR} \
				-DMKTEMP_COMMAND=/usr/bin/mktemp \
				-DSHELL_CMD=/bin/sh
CPPSCRIPTFLAGS_xdm-config=	-DXDMDIR=${XDMDIR} \
				-DXDMAUTHDIR=${XDMVARDIR} \
				-DXDMLOGDIR=${XDMLOGDIR} \
				-DXDMPIDDIR=${XDMPIDDIR} \
				-DXPM \
				-DSU="su -m"

CONFIGFILES=		GiveConsole TakeConsole \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling \
			${CPPSCRIPTS} 
FILES=			xorg-bw.xpm xorg.xpm
FILESDIR=		${XDMDIR}
FILESDIR_XFree86.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_XFree86bw.xpm=	${X11INCDIR}/X11/pixmaps
FILESMODE_GiveConsole=	${BINMODE}
FILESMODE_TakeConsole=	${BINMODE}
FILESMODE_Xsetup_0=	${BINMODE}
FILESMODE_Xsession=	${BINMODE}
FILESMODE_Xwilling=	${BINMODE}

CONFIGSYMLINKS=		Xservers.ws ${XDMDIR}/Xservers

.PATH:		${X11SRCDIR.xdm}/config

.include "../Makefile.xdm"

CPPSCRIPTFLAGS_xdm-config+=	-DCHOOSERPATH=${XDMCHOOSERPATH}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
