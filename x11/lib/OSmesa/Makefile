#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:43 lukem Exp $

NOLINT=		1	# XTODO: 3dnow.ln barfs on src/math/m_vertices.h

.include <bsd.own.mk>

LIB=		OSMesa

.PATH:		${X11SRCDIR.xc}/extras/Mesa/src
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/OSmesa
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/X86
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/array_cache
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/math
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast_setup
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/tnl

SUBDIR=		gen_matypes

SRCS=		osmesa.c accum.c api_arrayelt.c api_eval.c \
		api_loopback.c api_noop.c api_validate.c attrib.c \
		blend.c buffers.c clip.c colortab.c config.c \
		context.c convolve.c debug.c depth.c dlist.c \
		drawpix.c enable.c enums.c eval.c extensions.c \
		feedback.c fog.c get.c hash.c hint.c histogram.c \
		image.c imports.c light.c lines.c matrix.c mem.c \
		mmath.c pixel.c points.c polygon.c rastpos.c state.c \
		stencil.c texformat.c teximage.c texobj.c texstate.c \
		texstore.c texutil.c varray.c vtxfmt.c ac_context.c \
		ac_import.c m_debug_clip.c m_debug_norm.c \
		m_debug_xform.c m_debug_vertex.c m_eval.c m_matrix.c \
		m_translate.c m_vector.c m_vertices.c m_xform.c \
		s_aaline.c s_aatriangle.c s_accum.c s_alpha.c \
		s_alphabuf.c s_bitmap.c s_blend.c s_buffers.c \
		s_context.c s_copypix.c s_depth.c s_drawpix.c \
		s_feedback.c s_fog.c s_histogram.c s_imaging.c \
		s_lines.c s_logic.c s_masking.c s_pb.c s_pixeltex.c \
		s_points.c s_readpix.c s_scissor.c s_span.c \
		s_stencil.c s_texstore.c s_texture.c s_triangle.c \
		s_zoom.c ss_context.c ss_triangle.c ss_vb.c \
		t_array_api.c t_array_import.c t_context.c t_eval_api.c \
		t_imm_alloc.c t_imm_api.c t_imm_debug.c t_imm_dlist.c \
		t_imm_elt.c t_imm_eval.c t_imm_exec.c t_imm_fixup.c \
		t_pipeline.c t_vb_fog.c t_vb_light.c t_vb_normals.c \
		t_vb_points.c t_vb_render.c t_vb_texgen.c t_vb_texmat.c \
		t_vb_vertex.c common_x86.c common_x86_asm.S x86.c \
		x86_cliptest.S x86_vertex.S x86_xform2.S x86_xform3.S \
		x86_xform4.S mmx_blend.S 3dnow.c 3dnow_normal.S \
		3dnow_vertex.S 3dnow_xform1.S 3dnow_xform2.S \
		3dnow_xform3.S 3dnow_xform4.S sse.c sse_normal.S \
		sse_vertex.S sse_xform1.S sse_xform2.S sse_xform3.S \
		sse_xform4.S

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src \
		${X11FLAGS.THREADLIB} \
		-DGLXEXT -DGLX_USE_MESA \
		-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM

COMPILE.S=	${COMPILE.s}		# XXX don't want -traditional-cpp
AFLAGS+=	-DUSE_GAS -I. -I${X11SRCDIR.xc}/extras/Mesa/src/X86


GENMATYPESDIR!=	cd ${.CURDIR}/gen_matypes && ${PRINTOBJDIR}
GENMATYPES=	${GENMATYPESDIR}/gen_matypes

${GENMATYPES}:
	(cd ${.CURDIR}/gen_matypes && ${MAKE})

matypes.h: ${GENMATYPES}
	rm -f ${.TARGET}
	${GENMATYPES} > ${.TARGET}
CLEANFILES+=	matypes.h

DPSRCS+=	matypes.h


LDADD+=		-lGL
DPADD+=		${LIBGL}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
