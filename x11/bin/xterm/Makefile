#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:52 lukem Exp $

.include <bsd.own.mk>

PROG=	xterm
SRCS=	button.c charproc.c charsets.c cursor.c data.c doublechr.c \
	fontutils.c input.c menu.c misc.c print.c ptydata.c screen.c \
	scrollbar.c tabs.c util.c xstrings.c TekPrsTbl.c Tekproc.c VTPrsTbl.c \
	main.c charclass.c precompose.c wcwidth.c xutf8.c

BINOWN= root
BINMODE=4555

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/xterm \
		-I${DESTDIR}/${X11INCDIR}/freetype2 \
		-DSCROLLBAR_RIGHT -DOPT_WIDE_CHARS -DXRENDERFONT \
		-DPROJECTROOT=${X11ROOTDIR}

CPPFLAGS.main.c=-DUTMP -DUSE_TTY_GROUP ${X11FLAGS.VERSION}

APPDEFS=XTerm.ad XTerm-color.ad

BUILDSYMLINKS=	XTerm-col.ad XTerm-color.ad \
		termcap xterm.termcap \
		terminfo xterm.terminfo

FILESDIR=${X11LIBDIR}/etc
FILES=	xterm.termcap xterm.terminfo

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -ltermcap -lutil
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBTERMCAP} ${LIBUTIL}

SUBDIR=	resize uxterm

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
