#	$NetBSD: Makefile,v 1.5 2018/03/18 11:40:41 jmcneill Exp $

.include <bsd.own.mk>

PROG=	ctwm
SRCS=	add_window.c deftwmrc.c  events.c gram.tab.c gc.c \
	list.c util.c ctwm.c lex.c parse.c menus.c resize.c \
	version.c iconmgr.c cursor.c icons.c workmgr.c windowbox.c \
	clicktofocus.c vscreen.c session.c mwmhints.c gnome.c

YHEADER=1

CTWMCONFIGDIR=		${X11ETCDIR}/ctwm

CTWMDIR=		${X11SRCDIR}/external/mit/ctwm/dist
XPMDIR=			${X11INCDIR}/X11/pixmaps/ctwm

CPPFLAGS+=		-I. -I${CTWMDIR}/src -DHAVE_MKSTEMP \
			-DNO_ALLOCA -DCSRG_BASED -DUSEM4 -DGNOME -DXPM

CPPFLAGS+=		-DPIXMAP_DIRECTORY=\"${XPMDIR}\"
CPPFLAGS+=		-DCTWM_WELCOME_FILE=\"xpm:${X11INCDIR}/X11/pixmaps/NetBSD-inv.xpm\"

CPPFLAGS.parse.c=	-DSYSTEM_INIT_FILE=\"${CTWMCONFIGDIR}/system.ctwmrc\" \
			-DM4CMD=\"m4\"

FILESDIR=		${XPMDIR}

FILES=			3D_Expand15.xpm 3D_Iconify15.xpm 3D_Lightning15.xpm \
			3D_Menu15.xpm 3D_Resize15.xpm 3D_Zoom15.xpm 3dcircle.xpm \
			3ddimple.xpm 3ddot.xpm 3dfeet.xpm 3dleopard.xpm 3dpie.xpm \
			3dpyramid.xpm 3dslant.xpm IslandD.xpm IslandW.xpm LRom.xpm\
			LRom1.xpm arthur.xpm audio_editor.xpm background1.xpm \
			background2.xpm background3.xpm background4.xpm \
			background5.xpm background6.xpm background7.xpm \
			background8.xpm background9.xpm ball1.xpm ball10.xpm \
			ball11.xpm ball12.xpm ball2.xpm ball3.xpm ball4.xpm \
			ball5.xpm ball6.xpm ball7.xpm ball8.xpm ball9.xpm \
			cdrom1.xpm claude.xpm clipboard.xpm datebook.xpm \
			emacs.xpm ghostview.xpm hpterm.xpm mail0.xpm mail1.xpm \
			nothing.xpm nt1.xpm nt2.xpm pixmap.xpm postit.xpm \
			skull.xpm spider.xpm term.xpm unknown.xpm unknown1.xpm \
			unread.xpm welcome.xpm xarchie.xpm xcalc.xpm xcalc2.xpm \
			xedit.xpm xftp.xpm xgopher.xpm xgrab.xpm xhpcalc.xpm \
			xirc.xpm xmail.xpm xman.xpm xmosaic.xpm xnomail.xpm \
			xrn-compose.xpm xrn.goodnews.xpm xrn.nonews.xpm xrn.xpm \
			xterm.xpm system.ctwmrc

FILESDIR_system.ctwmrc = ${CTWMCONFIGDIR}


LDADD+=	-lXpm  -lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${CTWMDIR} ${CTWMDIR}/xpm

.include <bsd.files.mk>
.include <bsd.x11.mk>
.include <bsd.prog.mk>
