#	$NetBSD: Makefile,v 1.4.2.1 2009/05/13 18:53:23 jym Exp $

.include <bsd.own.mk>

# native only gives DEFAULTVT for i386 and amd64.
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
DEFAULTVT=vt05
.else
DEFAULTVT=	# defined empty
.endif

CPPSCRIPTS=	Xresources Xreset Xservers.ws Xsession Xstartup xdm-config 

CPPSCRIPTFLAGS_Xresources=	-DXPM -DBITMAPDIR=${X11INCDIR}/X11/pixmaps \
				-DXDM_PIXMAP=NetBSD.xpm \
				-DXDM_BWPIXMAP=NetBSD-bw.xpm
CPPSCRIPTFLAGS_Xreset=		-DBINDIR=${X11BINDIR} -DXDMCONFIGDIR=${XDMDIR}
CPPSCRIPTFLAGS_Xservers.ws=	-DBINDIR=${X11BINDIR} -DDEFAULTVT=${DEFAULTVT}
CPPSCRIPTFLAGS_Xsession=	-DBINDIR=${X11BINDIR} \
				-DMKTEMP_COMMAND=/usr/bin/mktemp \
				-DSHELL_CMD=/bin/sh
CPPSCRIPTFLAGS_Xstartup=	-DBINDIR=${X11BINDIR} -DXDMCONFIGDIR=${XDMDIR}
CPPSCRIPTFLAGS_xdm-config=	-DXDMDIR=${XDMDIR} \
				-DXDMAUTHDIR=${XDMVARDIR} \
				-DXDMLOGDIR=${XDMLOGDIR} \
				-DXDMPIDDIR=${XDMPIDDIR} \
				-DXPM \
				-DSU="su -m"

CONFIGFILES=		GiveConsole TakeConsole \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling \
			${CPPSCRIPTS} 
FILES=			xorg-bw.xpm xorg.xpm \
			NetBSD-bw.xpm NetBSD-inv.xpm NetBSD.xpm
FILESDIR=		${XDMDIR}
FILESDIR_xorg.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_xorg-bw.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD-bw.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD-inv.xpm=${X11INCDIR}/X11/pixmaps
FILESMODE_GiveConsole=	${BINMODE}
FILESMODE_TakeConsole=	${BINMODE}
FILESMODE_Xreset=	${BINMODE}
FILESMODE_Xsetup_0=	${BINMODE}
FILESMODE_Xsession=	${BINMODE}
FILESMODE_Xstartup=	${BINMODE}
FILESMODE_Xwilling=	${BINMODE}

CONFIGSYMLINKS=		Xservers.ws ${XDMDIR}/Xservers

.PATH:		${X11SRCDIR.xdm}/config

.include "../Makefile.xdm"

CPPSCRIPTFLAGS_xdm-config+=	-DCHOOSERPATH=${XDMCHOOSERPATH}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
