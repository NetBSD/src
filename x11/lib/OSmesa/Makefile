#	$NetBSD: Makefile,v 1.15 2005/01/21 05:15:36 rtr Exp $

NOLINT=		1	# XTODO: 3dnow.ln barfs on src/math/m_vertices.h

.include <bsd.own.mk>

LIB=		OSMesa

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/OSmesa
SRCS=	osmesa.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src
SRCS+=	accum.c api_arrayelt.c api_eval.c api_loopback.c api_noop.c \
	api_validate.c attrib.c blend.c buffers.c clip.c colortab.c \
	context.c convolve.c debug.c depth.c dlist.c drawpix.c \
	enable.c enums.c eval.c extensions.c feedback.c fog.c get.c \
	hash.c hint.c histogram.c image.c imports.c light.c lines.c \
	matrix.c mmath.c pixel.c points.c polygon.c rastpos.c \
	state.c stencil.c texformat.c teximage.c texobj.c \
	texstate.c texstore.c texutil.c varray.c vtxfmt.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/array_cache
SRCS+=	ac_context.c ac_import.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/math
SRCS+=	m_debug_clip.c m_debug_norm.c m_debug_xform.c m_eval.c \
	m_matrix.c m_translate.c m_vector.c m_xform.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/swrast
SRCS+=	s_aaline.c s_aatriangle.c s_accum.c s_alpha.c s_alphabuf.c \
	s_bitmap.c s_blend.c s_buffers.c s_context.c s_copypix.c \
	s_depth.c s_drawpix.c s_feedback.c s_fog.c s_histogram.c \
	s_imaging.c s_lines.c s_logic.c s_masking.c s_pixeltex.c \
	s_points.c s_readpix.c s_span.c s_stencil.c s_texstore.c \
	s_texture.c s_triangle.c s_zoom.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/swrast_setup
SRCS+=	ss_context.c ss_triangle.c ss_vb.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/tnl
SRCS+=	t_array_api.c t_array_import.c t_context.c t_eval_api.c \
	t_imm_alloc.c t_imm_api.c t_imm_debug.c t_imm_dlist.c \
	t_imm_elt.c t_imm_eval.c t_imm_exec.c t_imm_fixup.c \
	t_pipeline.c t_vb_fog.c t_vb_light.c t_vb_normals.c \
	t_vb_points.c t_vb_render.c t_vb_texgen.c t_vb_texmat.c \
	t_vb_vertex.c

.if ${X11DRI} != "no"
CPPFLAGS+=	${X11FLAGS.DRI}
.endif

# XXX removed in xf44 if not found to be needed remove by > xf44
# config.c, m_debug_vertex.c, m_vertices.c, mem.c, s_pb.c, s_scissor.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src \
		${X11FLAGS.THREADS} \
		-DGLXEXT -DGLX_USE_MESA

.if ${MACHINE_ARCH} == "i386"
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/X86
SRCS+=		common_x86.c common_x86_asm.S x86.c x86_cliptest.S \
		x86_xform2.S x86_xform3.S x86_xform4.S \
		mmx_blend.S \
		3dnow.c 3dnow_normal.S 3dnow_xform1.S \
		3dnow_xform2.S 3dnow_xform3.S 3dnow_xform4.S \
		sse.c sse_normal.S sse_xform1.S \
		sse_xform2.S sse_xform3.S sse_xform4.S

# XXX removed in xf44 if not found to be needed remove by > xf44
# 3dnow_vertex.S, sse_vertex.S, x86_vertex.S

CPPFLAGS+=	-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM
COMPILE.S=	${COMPILE.s}		# XXX don't want -traditional-cpp
AFLAGS+=	-DUSE_GAS -I. -I${X11SRCDIR.xc}/extras/Mesa/src/X86
.endif

.if ${MACHINE_CPU} == "sh5"
COPTS.s_triangle.c=	-O0
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/SPARC
SRCS+=		sparc.c xform.S clip.S norm.S
CPPFLAGS+=	-DUSE_SPARC_ASM
COMPILE.S=	${COMPILE.s}		# XXX don't want -traditional-cpp
AFLAGS+=	-DUSE_GAS -I. -I${X11SRCDIR.xc}/extras/Mesa/src/SPARC
.endif

.include "${NETBSDSRCDIR}/x11/tools/gen_matypes/Makefile.gen_matypes"

matypes.h: ${GEN_MATYPES}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${GEN_MATYPES} > ${.TARGET}
CLEANFILES+=	matypes.h

DPSRCS+=	matypes.h


LIBDPLIBS=	GL ${.CURDIR}/../GL

.include <bsd.x11.mk>
.include <bsd.lib.mk>
