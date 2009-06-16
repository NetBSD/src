#	$NetBSD: libmesa.mk,v 1.4 2009/06/16 00:26:11 mrg Exp $
#
# Consumer of this Makefile should set MESA_SRC_MODULES.

INCLUDES.all=	. glapi main

# Main sources
PATHS.main=	main
INCLUDES.main=	shader
SRCS.main= \
	api_arrayelt.c \
	api_exec.c \
	api_loopback.c \
	api_noop.c \
	api_validate.c \
	accum.c \
	attrib.c \
	arrayobj.c \
	blend.c \
	bufferobj.c \
	buffers.c \
	clear.c \
	clip.c \
	colortab.c \
	context.c \
	convolve.c \
	debug.c \
	depth.c \
	depthstencil.c \
	dlist.c \
	dlopen.c \
	drawpix.c \
	enable.c \
	enums.c \
	eval.c \
	execmem.c \
	extensions.c \
	fbobject.c \
	feedback.c \
	ffvertex_prog.c \
	fog.c \
	framebuffer.c \
	get.c \
	getstring.c \
	hash.c \
	hint.c \
	histogram.c \
	image.c \
	imports.c \
	light.c \
	lines.c \
	matrix.c \
	mipmap.c \
	mm.c \
	multisample.c \
	pixel.c \
	pixelstore.c \
	points.c \
	polygon.c \
	queryobj.c \
	rastpos.c \
	rbadaptors.c \
	readpix.c \
	renderbuffer.c \
	scissor.c \
	shaders.c \
	state.c \
	stencil.c \
	texcompress.c \
	texcompress_s3tc.c \
	texcompress_fxt1.c \
	texenv.c \
	texenvprogram.c \
	texformat.c \
	texgen.c \
	teximage.c \
	texobj.c \
	texparam.c \
	texrender.c \
	texstate.c \
	texstore.c \
	varray.c \
	vtxfmt.c

# Math sources
PATHS.math=	math
SRCS.math= \
	m_debug_clip.c \
	m_debug_norm.c \
	m_debug_xform.c \
	m_eval.c \
	m_matrix.c \
	m_translate.c \
	m_vector.c \
	m_xform.c

# Software raster sources
PATHS.swrast=	swrast swrast_setup
INCLUDES.swrast=	shader
SRCS.swrast= \
	s_aaline.c \
	s_aatriangle.c \
	s_accum.c \
	s_alpha.c \
	s_atifragshader.c \
	s_bitmap.c \
	s_blend.c \
	s_blit.c \
	s_buffers.c \
	s_copypix.c \
	s_context.c \
	s_depth.c \
	s_drawpix.c \
	s_feedback.c \
	s_fog.c \
	s_fragprog.c \
	s_imaging.c \
	s_lines.c \
	s_logic.c \
	s_masking.c \
	s_points.c \
	s_readpix.c \
	s_span.c \
	s_stencil.c \
	s_texcombine.c \
	s_texfilter.c \
	s_texstore.c \
	s_triangle.c \
	s_zoom.c

# swrast_setup
.PATH:		${X11SRCDIR.MesaLib}/src/mesa/swrast_setup
SRCS.ss= \
	ss_context.c \
	ss_triangle.c

# TNL sources
PATHS.tnl=	tnl
INCLUDES.tnl=	shader
SRCS.tnl= \
	t_context.c \
	t_pipeline.c \
	t_draw.c \
	t_rasterpos.c \
	t_vb_program.c \
	t_vb_render.c \
	t_vb_texgen.c \
	t_vb_texmat.c \
	t_vb_vertex.c \
	t_vb_cull.c \
	t_vb_fog.c \
	t_vb_light.c \
	t_vb_normals.c \
	t_vb_points.c \
	t_vp_build.c \
	t_vertex.c \
	t_vertex_sse.c \
	t_vertex_generic.c

# VBO sources
PATHS.vbo=	vbo
SRCS.vbo= \
	vbo_context.c \
	vbo_exec.c \
	vbo_exec_api.c \
	vbo_exec_array.c \
	vbo_exec_draw.c \
	vbo_exec_eval.c \
	vbo_rebase.c \
	vbo_split.c \
	vbo_split_copy.c \
	vbo_split_inplace.c \
	vbo_save.c \
	vbo_save_api.c \
	vbo_save_draw.c \
	vbo_save_loopback.c

COPTS.vbo_save_draw.c=	-Wno-error

# Shader sources
PATHS.shader=	shader shader/grammar
INCLUDES.shader=	shader/slang shader/grammar
SRCS.shader= \
	arbprogparse.c \
	arbprogram.c \
	atifragshader.c \
	grammar_mesa.c \
	nvfragparse.c \
	nvprogram.c \
	nvvertparse.c \
	program.c \
	prog_cache.c \
	prog_debug.c \
	prog_execute.c \
	prog_instruction.c \
	prog_noise.c \
	prog_parameter.c \
	prog_print.c \
	prog_statevars.c \
	prog_uniform.c \
	programopt.c \
	shader_api.c \

# Shader language sources
PATHS.slang=	shader/slang
INCLUDES.slang=	shader shader/grammar
SRCS.slang= \
	slang_builtin.c \
	slang_codegen.c \
	slang_compile.c \
	slang_compile_function.c \
	slang_compile_operation.c \
	slang_compile_struct.c \
	slang_compile_variable.c \
	slang_emit.c \
	slang_ir.c \
	slang_label.c \
	slang_link.c \
	slang_log.c \
	slang_mem.c \
	slang_preprocess.c \
	slang_print.c \
	slang_simplify.c \
	slang_storage.c \
	slang_typeinfo.c \
	slang_vartable.c \
	slang_utility.c

# GL API sources
PATHS.glapi=	glapi main
SRCS.glapi= \
	dispatch.c \
	glapi.c \
	glapi_getproc.c \
	glthread.c

# Common driver sources
PATHS.common=	drivers/common
INCLUDES.common=	shader
SRCS.common= \
	driverfuncs.c


# OSMesa driver sources
PATHS.osmesa=	drivers/osmesa
INCLUDES.osmesa=	shader
SRCS.osmesa= \
	osmesa.c

.for _mod_ in ${MESA_SRC_MODULES}

SRCS+=	${SRCS.${_mod_}}

. for _path_ in ${PATHS.${_mod_}}
.PATH:	${X11SRCDIR.MesaLib}/src/mesa/${_path_}
. endfor

. for _path_ in ${INCLUDES.${_mod_}}
CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/${_path_}
. endfor

.endfor

.for _path_ in ${INCLUDES.all}
CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/${_path_}
.endfor

LIBDPLIBS=	m	${NETBSDSRCDIR}/lib/libm
