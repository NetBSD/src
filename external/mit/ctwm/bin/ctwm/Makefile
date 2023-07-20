#	$NetBSD: Makefile,v 1.12 2023/07/05 08:39:24 nia Exp $

.include <bsd.own.mk>

PROG=	ctwm
SRCS=	add_window.c animate.c clargs.c clicktofocus.c \
	colormaps.c ctopts.c ctwm_main.c ctwm_shutdown.c ctwm_takeover.c \
	cursor.c drawing.c event_core.c event_handlers.c event_names.c \
	event_utils.c functions.c functions_icmgr_wsmgr.c \
	functions_identify.c functions_misc.c functions_warp.c \
	functions_win.c functions_win_moveresize.c functions_workspaces.c \
	gc.c iconmgr.c icons.c icons_builtin.c image.c image_bitmap.c \
	image_bitmap_builtin.c image_xwd.c list.c mask_screen.c menus.c \
	mwmhints.c occupation.c otp.c parse.c parse_be.c parse_yacc.c \
	r_area.c r_area_list.c r_layout.c signals.c session.c util.c \
	vscreen.c win_decorations.c win_decorations_init.c win_iconify.c \
	win_ops.c win_regions.c win_resize.c win_ring.c win_utils.c \
	workspace_config.c workspace_manager.c workspace_utils.c \
	xparsegeometry.c xrandr.c \
	ctwm_atoms.c deftwmrc.c version.c \
	image_xpm.c parse_m4.c ewmh.c ewmh_atoms.c ctwm_wrap.c

SRCS+=	repl_str.c

SRCS+=	gram.tab.c lex.c

YHEADER=1

CTWMCONFIGDIR=		${X11ETCDIR}/ctwm

CTWMDIR=		${X11SRCDIR}/external/mit/ctwm/dist
XPMDIR=			${X11INCDIR}/X11/pixmaps/ctwm

CPPFLAGS+=		-I. -I${CTWMDIR} -I${CTWMDIR}/gen \
			-DUSE_XPM -DUSE_M4 -DUSE_EWMH -DUSE_SREGEX \
			-DUSE_SESSION -DUSE_XRANDR \
			-DPIXMAP_DIRECTORY=\"${XPMDIR}\" \
			-DSYSTEM_INIT_FILE=\"${CTWMCONFIGDIR}/system.ctwmrc\" \
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
			xterm.xpm

LDADD+=	-lXpm  -lXmu -lXt -lSM -lICE -lXext -lXrandr -lX11
DPADD+=	${LIBXPM} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBXRANDR} ${LIBX11}

.PATH:	${CTWMDIR} ${CTWMDIR}/xpm ${CTWMDIR}/ext ${CTWMDIR}/gen

.include <bsd.files.mk>
.include <bsd.x11.mk>
.include <bsd.prog.mk>

COPTS+=			${${ACTIVE_CC} == "gcc":? -fcommon :}
