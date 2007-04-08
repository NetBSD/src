#	$NetBSD: Makefile,v 1.20 2007/04/08 09:36:35 scw Exp $

NOLINT=		1	# XTODO: 3dnow.ln barfs on src/math/m_vertices.h

.include <bsd.own.mk>

LIB=		OSMesa

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/drivers/common
SRCS+=	driverfuncs.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/drivers/osmesa
SRCS+=	osmesa.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/main
SRCS+=	accum.c api_arrayelt.c api_loopback.c api_noop.c \
	api_validate.c attrib.c blend.c buffers.c clip.c colortab.c \
	context.c convolve.c debug.c depth.c dlist.c drawpix.c \
	enable.c enums.c eval.c extensions.c feedback.c fog.c get.c \
	hash.c hint.c histogram.c image.c imports.c light.c lines.c \
	matrix.c occlude.c pixel.c points.c polygon.c rastpos.c \
	state.c stencil.c texcompress.c texcompress_fxt1.c texcompress_s3tc.c \
	texformat.c teximage.c texobj.c texstate.c texstore.c \
	varray.c vtxfmt.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/array_cache
SRCS+=	ac_context.c ac_import.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/math
SRCS+=	m_debug_clip.c m_debug_norm.c m_debug_xform.c m_eval.c \
	m_matrix.c m_translate.c m_vector.c m_xform.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/shader
SRCS+=	arbprogram.c arbfragparse.c arbprogparse.c arbvertparse.c \
	bufferobj.c grammar_mesa.c \
	nvfragparse.c nvprogram.c nvvertexec.c nvvertparse.c \
	program.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/swrast
SRCS+=	s_aaline.c s_aatriangle.c s_accum.c s_alpha.c s_alphabuf.c \
	s_auxbuffer.c \
	s_bitmap.c s_blend.c s_buffers.c s_context.c s_copypix.c \
	s_depth.c s_drawpix.c s_feedback.c s_fog.c s_imaging.c \
	s_lines.c s_logic.c s_masking.c s_nvfragprog.c s_pixeltex.c \
	s_points.c s_readpix.c s_span.c s_stencil.c s_texstore.c \
	s_texture.c s_triangle.c s_zoom.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/swrast_setup
SRCS+=	ss_context.c ss_triangle.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/tnl
SRCS+=	t_array_api.c t_array_import.c t_context.c \
	t_pipeline.c t_save_api.c t_save_loopback.c t_save_playback.c \
	t_vb_fog.c t_vb_light.c t_vb_normals.c t_vb_points.c \
	t_vb_program.c t_vb_render.c t_vb_texgen.c t_vb_texmat.c \
	t_vb_vertex.c t_vertex.c t_vertex_c.c t_vtx_api.c t_vtx_eval.c \
	t_vtx_exec.c t_vtx_generic.c t_save_playback.c

.if ${X11DRI} != "no"
CPPFLAGS+=	${X11FLAGS.DRI}
.endif

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/main \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/shader \
		${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION} \
		-DGLXEXT -DGLX_USE_MESA

.if ${MACHINE_ARCH} == "i386"
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/mesa/x86
SRCS+=		common_x86.c common_x86_asm.S x86.c x86_cliptest.S \
		x86_xform2.S x86_xform3.S x86_xform4.S \
		mmx_blend.S \
		3dnow.c 3dnow_normal.S 3dnow_xform1.S \
		3dnow_xform2.S 3dnow_xform3.S 3dnow_xform4.S \
		sse.c sse_normal.S sse_xform1.S \
		sse_xform2.S sse_xform3.S sse_xform4.S \
		t_vtx_x86.c t_vtx_x86_gcc.S

CPPFLAGS+=	-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM
COMPILE.S=	${COMPILE.s}		# XXX don't want -traditional-cpp
AFLAGS+=	-DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM
AFLAGS+=	-DUSE_GAS -I. -I${X11SRCDIR.xc}/extras/Mesa/src/mesa/x86
.endif

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/mesa/sparc
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

LIBDPLIBS=\
	GL	${.CURDIR}/../GL

LDADD+=		-lm
DPADD+=		${LIBM}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
