#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	x11perf
SRCS=	x11perf.c bitmaps.c do_tests.c do_simple.c do_rects.c do_valgc.c \
	do_lines.c do_segs.c do_dots.c do_windows.c do_movewin.c do_text.c \
	do_blt.c do_arcs.c do_tris.c do_complex.c do_traps.c

CPPFLAGS+=	-DMITSHM -DXFT -DXRENDER -I${DESTDIR}/${X11INCDIR}/freetype2

MAN=	Xmark.1 x11perf.1 x11perfcomp.1

LNFILES=x11pcomp.man x11perfcomp.man \
	x11pcomp.cpp x11perfcomp.cpp \

CPPSCRIPTS=		x11perfcomp
CPPSCRIPTFLAGS=		-DLIBPATH=${X11LIBDIR}/x11perfcomp
SCRIPTS=		x11perfcomp Xmark.sh \
			fillblnk.sh perfboth.sh perfratio.sh
SCRIPTSDIR_fillblnk.sh=	${X11LIBDIR}/x11perfcomp
SCRIPTSDIR_perfboth.sh=	${X11LIBDIR}/x11perfcomp
SCRIPTSDIR_perfratio.sh=${X11LIBDIR}/x11perfcomp

LDADD+=	-lXft -lfontconfig -lfreetype -lexpat -lXrender
DPADD+=	${LIBXFT} ${LIBFONTCONFIG} ${LIBFREETYPE} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	-lXmuu -lXext -lX11 -lm
DPADD+=	${LIBXMUU} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
