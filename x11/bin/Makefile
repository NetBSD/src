#	$NetBSD: Makefile,v 1.4 2003/09/14 22:33:31 lukem Exp $

SUBDIR=	appres bdftopcf bdftruncate beforelight bitmap cxpm dpsexec dpsinfo \
	editres fc-cache fc-list fslsfonts fstobdf iceauth \
	ico imake lbxproxy listres lndir luit makepsres mkcfm mkfontdir \
	mkfontscale oclock proxymngr pswrap rgb rstart scripts setxkbmap \
	showfont smproxy sxpm texteroids twm ucs2any viewres x11perf xauth \
	xbiff xcalc xclock xcmsdb xclipboard xcutsel xconsole xditview \
	xdm xdpyinfo xedit xev xeyes xf86dga xfd xfs xfindproxy xfontsel \
	xfsinfo xfwp xgamma xgc xhost xinit xkbevd \
	xkbcomp xkbprint xkbutils xkill xload xlsatoms xlsclients \
	xlsfonts xlogo xmag xman xmessage xmh xmkmf xmodmap xprop xrandr xrdb \
	xrefresh xset xsetmode xsetpointer xsetroot xsm xstdcmap \
	xterm xtrap xvidtune xvinfo xwd xwininfo xwud

.if ${MACHINE_ARCH} != "vax"	# XXX: vax is missing ceilf (etc) in libm
SUBDIR+=glxgears glxinfo 
.endif

SUBDIR+=ssh-askpass xsystrace

.include <bsd.subdir.mk>
