#	$NetBSD: Makefile,v 1.4 2009/09/19 04:54:04 snj Exp $

.include <bsd.own.mk>

PROG=	xterm
SRCS=	button.c charproc.c charsets.c cursor.c data.c doublechr.c \
	fontutils.c input.c linedata.c menu.c misc.c print.c ptydata.c \
	screen.c scrollback.c scrollbar.c tabs.c util.c xstrings.c \
	TekPrsTbl.c Tekproc.c VTPrsTbl.c main.c charclass.c precompose.c \
	wcwidth.c xutf8.c cachedGCs.c xtermcap.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}} \
		-I${DESTDIR}${X11INCDIR}/freetype2 \
		-DSCROLLBAR_RIGHT -DOPT_WIDE_CHARS \
		-DXRENDERFONT -DXFREE86_FT2 -DOPT_PASTE64 \
		-DPROJECTROOT=${X11ROOTDIR}

CPPFLAGS.main.c=-DUTMP -DUSE_TTY_GROUP ${X11FLAGS.VERSION}

APPDEFS=XTerm.ad XTerm-color.ad

BUILDSYMLINKS=	XTerm-col.ad XTerm-color.ad

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -ltermcap -lutil
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBTERMCAP} ${LIBUTIL}

SUBDIR=	resize uxterm

.PATH:	${X11SRCDIR.${PROG}}

.include "Makefile.inc"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
