#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:05 lukem Exp $

.include <bsd.own.mk>

LIB=		mesa

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src
SRCS+=		accum.c api_arrayelt.c api_eval.c api_loopback.c \
		api_noop.c api_validate.c attrib.c blend.c buffers.c \
		clip.c colortab.c config.c context.c convolve.c \
		debug.c depth.c dispatch.c dlist.c drawpix.c enable.c \
		enums.c eval.c extensions.c feedback.c fog.c get.c \
		glapi.c glthread.c hash.c hint.c histogram.c image.c \
		imports.c light.c lines.c matrix.c mem.c mmath.c \
		pixel.c points.c polygon.c rastpos.c state.c \
		stencil.c texformat.c teximage.c texobj.c texstate.c \
		texstore.c texutil.c varray.c vtxfmt.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/math
SRCS+=		m_debug_clip.c m_debug_norm.c m_debug_xform.c \
		m_debug_vertex.c m_eval.c m_matrix.c m_translate.c \
		m_vector.c m_vertices.c m_xform.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/array_cache
SRCS+=		ac_context.c ac_import.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast
SRCS+=		s_aaline.c s_aatriangle.c s_accum.c s_alpha.c \
		s_alphabuf.c s_bitmap.c s_blend.c s_buffers.c \
		s_context.c s_copypix.c s_depth.c s_drawpix.c \
		s_feedback.c s_fog.c s_histogram.c s_imaging.c \
		s_lines.c s_logic.c s_masking.c s_pb.c s_pixeltex.c \
		s_points.c s_readpix.c s_scissor.c s_span.c \
		s_stencil.c s_texstore.c s_texture.c s_triangle.c \
		s_zoom.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast_setup
SRCS+=		ss_context.c ss_triangle.c ss_vb.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/tnl
SRCS+=		t_array_api.c t_array_import.c t_context.c t_eval_api.c \
		t_imm_alloc.c t_imm_api.c t_imm_debug.c t_imm_dlist.c \
		t_imm_elt.c t_imm_eval.c t_imm_exec.c t_imm_fixup.c \
		t_pipeline.c t_vb_fog.c t_vb_light.c t_vb_normals.c \
		t_vb_points.c t_vb_render.c t_vb_texgen.c t_vb_texmat.c \
		t_vb_vertex.c

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/X
.PATH:		${X11SRCDIR.xc}/programs/Xserver/GL/mesa/src/X
.PATH:		${X11SRCDIR.xc}/lib/GL/glx
SRCS+=		compsize.c xf86glx.c xf86glx_util.c xm_api.c xm_dd.c \
		xm_line.c xm_span.c xm_tri.c

.PATH:		${X11SRCDIR.xc}/programs/Xserver/GL/mesa/GLcore
SRCS+=		GLcoremodule.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
 		-I${X11SRCDIR.xc}/extras/Mesa/include \
		-I${X11SRCDIR.xc}/lib/GL/include \
 		-I${X11SRCDIR.xc}/extras/Mesa/src \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
 		-I${X11SRCDIR.xc}/extras/Mesa/src/X \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/glx \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/include \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/mesa/src/X

CPPFLAGS+=	-DIN_MODULE -DXFree86Module -DGLXEXT -DGLX_USE_MESA

.include <bsd.lib.mk>
.include <bsd.x11.mk>
