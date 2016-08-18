#	$NetBSD: Makefile,v 1.1 2016/08/18 07:48:14 mrg Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=	glamor

.PATH:	${X11SRCDIR.xorg-server}/${LIB}
SRCS= \
	glamor.c \
	glamor_copy.c \
	glamor_core.c \
	glamor_dash.c \
	glamor_font.c \
	glamor_glx.c \
	glamor_composite_glyphs.c \
	glamor_image.c \
	glamor_lines.c \
	glamor_segs.c \
	glamor_render.c \
	glamor_gradient.c \
	glamor_prepare.c \
	glamor_program.c \
	glamor_rects.c \
	glamor_spans.c \
	glamor_text.c \
	glamor_transfer.c \
	glamor_transform.c \
	glamor_trapezoid.c \
	glamor_triangles.c \
	glamor_addtraps.c \
	glamor_glyphblt.c \
	glamor_points.c \
	glamor_pixmap.c \
	glamor_largepixmap.c \
	glamor_picture.c \
	glamor_vbo.c \
	glamor_window.c \
	glamor_fbo.c \
	glamor_compositerects.c \
	glamor_utils.c \
	glamor_sync.c
SRCS+= \
	glamor_xv.c
SRCS+= \
	glamor_egl_stubs.c
SRCS.EGL= \
	glamor_egl.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=      ${X11FLAGS.DIX}

INCSDIR=	${X11INCDIR}/xorg
INCS=		glamor.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
