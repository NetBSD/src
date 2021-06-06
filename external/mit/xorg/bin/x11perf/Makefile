#	$NetBSD: Makefile,v 1.4.10.1 2021/06/06 20:30:45 cjep Exp $

.include <bsd.own.mk>

PROG=	x11perf
SRCS=	x11perf.c bitmaps.c do_tests.c do_simple.c do_rects.c do_valgc.c \
	do_lines.c do_segs.c do_dots.c do_windows.c do_movewin.c do_text.c \
	do_blt.c do_arcs.c do_tris.c do_complex.c do_traps.c

CPPFLAGS+=	-DMITSHM -DXRENDER
CPPFLAGS+=	-DXFT -DXFREE86_FT2 -I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2/freetype

MAN=	Xmark.1 x11perf.1 x11perfcomp.1

x11perfcomp: x11perfcomp.in
	${TOOL_SED} \
		-e "s,@x11perfcompdir@,${X11LIBDIR}/x11perfcomp,;" \
		-e "s,@MKTEMP@,/usr/bin/mktemp,;" \
		< ${.ALLSRC} > ${.TARGET}

SCRIPTS=		x11perfcomp Xmark \
			fillblnk perfboth perfratio
SCRIPTSDIR_fillblnk.sh=	${X11LIBDIR}/x11perfcomp
SCRIPTSDIR_perfboth.sh=	${X11LIBDIR}/x11perfcomp
SCRIPTSDIR_perfratio.sh=${X11LIBDIR}/x11perfcomp

CLEANFILES+=	x11perfcomp

LDADD+=	-lXft ${XLIBLDADD_FC} -lexpat -lXrender
DPADD+=	${LIBXFT} ${XLIBDPADD_FC} ${LIBEXPAT} ${LIBXRENDER}
LDADD+=	${XLIBLDADD_XMUU}
DPADD+=	${XLIBDPADD_XMUU} 
LDADD+=	-lm
DPADD+= ${LIBM}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
