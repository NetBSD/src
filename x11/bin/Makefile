#	$NetBSD: Makefile,v 1.5.2.1 2004/04/29 05:46:01 jmc Exp $

SUBDIR=	appres bdftopcf bdftruncate beforelight bitmap \
	ccmakedep cleanlinks cxpm dpsexec dpsinfo editres \
	fc-cache fc-list fonttosfnt fslsfonts fstobdf gccmakedep \
	iceauth ico imake lbxproxy listres lndir luit \
	makedepend makeg makepsres makestrs mergelib \
	mkcfm mkdirhier mkfontdir mkfontscale mkhtmlindex \
	oclock proxymngr pswrap revpath rgb rman rstart \
	scripts setxkbmap showfont smproxy ssh-askpass sxpm \
	texteroids twm ucs2any viewres \
	x11perf xauth xbiff xcalc xclipboard xclock xcmsdb \
	xconsole xcutsel xditview xdm xdpyinfo xedit xev xeyes \
	xf86dga xfd xfindproxy xfontsel xfs xfsinfo xfwp \
	xgamma xgc xhost xinit xkbcomp xkbevd xkbprint xkbutils \
	xkill xload xlogo xlsatoms xlsclients xlsfonts xmag xman \
	xmessage xmh xmkmf xmodmap xprop xrandr xrdb xrefresh xset \
	xsetmode xsetpointer xsetroot xsm xstdcmap xsystrace xterm \
	xtrap xvidtune xvinfo xwd xwininfo xwud

.if ${MACHINE_ARCH} != "vax"	# XXX: vax is missing ceilf (etc) in libm
SUBDIR+=glxgears glxinfo 
.endif

.include <bsd.subdir.mk>
