#	$NetBSD: Makefile,v 1.18 2019/03/06 05:59:25 mrg Exp $

.include <bsd.own.mk>

# DEFAULTVT for all arches that are able to switch virtual consoles
# while in X (currently this needs WSDISPLAY_COMPAT_USL and
# WSDISPLAY_COMPAT_PCVT, this might change soon)
.if	   ${MACHINE_ARCH} == "x86_64" \
	|| ${MACHINE_ARCH} == "cats" \
	|| ${MACHINE_ARCH} == "hpcmips" \
	|| ${MACHINE_ARCH} == "hpcsh" \
	|| ${MACHINE_ARCH} == "hppa" \
	|| ${MACHINE_ARCH} == "i386" \
	|| ${MACHINE_ARCH} == "netwinder" \
	|| ${MACHINE_ARCH} == "shark" \
	|| ${MACHINE_ARCH} == "sparc" \
	|| ${MACHINE_ARCH} == "sparc64" \
	|| ${MACHINE_ARCH} == "zaurus"
DEFAULTVT=vt05
.else
DEFAULTVT=	# defined empty
.endif

UUDECODE_FILES=	NetBSD-flag.png
SEDSCRIPTS=	Xresources Xreset Xservers.ws Xsession Xstartup xdm-config 
.for f in ${SEDSCRIPTS}
FILESBUILD_$f=yes
.endfor

.SUFFIXES: .in

.in:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_SED} \
		-e '/ifdef XPM/d' -e '/endif \/\* XPM \*\//d' \
		-e 's,BITMAPDIR,${X11INCDIR}/X11/pixmaps,' \
		-e 's,XDM_PIXMAP,NetBSD.xpm,' \
		-e 's,XDM_BWPIXMAP,NetBSD-bw.xpm,' \
		-e 's,BINDIR,$(X11BINDIR),' \
		-e 's,DEFAULTVT,$(DEFAULTVT),' \
		-e 's,XDMDIR,$(XDMDIR),' \
		-e 's,XDMLOGDIR,$(XDMLOGDIR),' \
		-e 's,XDMXAUTHDIR,$(XDMVARDIR),' \
		-e 's,XDMPIDDIR,$(XDMPIDDIR),' \
		-e 's,XDMCONFIGDIR,$(XDMDIR),' \
		-e 's,XDMSCRIPTDIR,$(XDMDIR),' \
		-e 's,SU,su -m,' \
		-e 's,CHOOSERPATH,$(XDMCHOOSERPATH),' \
		-e 's,SHELL_CMD,/bin/sh,' \
		-e 's,UTMP_FILE,$(XDMPIDDIR)/utmp,' \
		-e 's,UTMP_FLAG,-u $(XDMPIDDIR)/utmp,' \
		-e 's,WTMP_FILE,$(XDMLOGDIR)/wtmp,' \
		-e 's,WTMP_FLAG,-w $(XDMLOGDIR)/wtmp,' \
		-e 's,MKTEMP_COMMAND,/usr/bin/mktemp,' \
		-e 's,XCOMM,\#,' \
		-e 's,XHASH,\#,' \
		-e 's,NLBS$$,\\n\\,' \
		-e 's,BS$$,\\,' \
		-e 's,/\*\*/,,g' \
		< ${.IMPSRC} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	${SEDSCRIPTS} ${SEDSCRIPTS:=.tmp}

CONFIGFILES+=		GiveConsole TakeConsole \
			Xaccess Xservers.fs Xsession Xsetup_0 Xwilling
FILES+=			xorg-bw.xpm xorg.xpm \
			NetBSD-bw.xpm NetBSD-inv.xpm NetBSD.xpm	\
			NetBSD-flag.png NetBSD-flag1.xpm NetBSD-flag2.xpm \
			${SEDSCRIPTS} 
FILESDIR=		${XDMDIR}
FILESDIR_xorg.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_xorg-bw.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD-flag.png=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD-flag1.xpm=	${X11INCDIR}/X11/pixmaps
FILESDIR_NetBSD-flag2.xpm=	${X11INCDIR}/X11/pixmaps
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

.include <bsd.x11.mk>
.include <bsd.prog.mk>
