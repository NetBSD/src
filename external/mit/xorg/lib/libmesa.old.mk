#	$NetBSD: libmesa.old.mk,v 1.3 2023/07/15 21:24:46 rjs Exp $
#
# Consumer of this Makefile should set MESA_SRC_MODULES.

CPPFLAGS.ac_surface.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=enum-conversion :}

# The source file lists derived from src/mesa/Makefile.sources.
# Please keep the organization in line with those files.

# Main sources
PATHS.main=	mesa/main ../../src/mesa/main ../../src/mapi/glapi
INCLUDES.main=	glsl mesa/main ../../src/compiler/nir
SRCS.main= \
	accum.c \
	api_arrayelt.c \
	api_loopback.c \
	api_exec.c \
	arbprogram.c \
	arrayobj.c \
	atifragshader.c \
	attrib.c \
	barrier.c \
	bbox.c \
	blend.c \
	blit.c \
	bufferobj.c \
	buffers.c \
	clear.c \
	clip.c \
	colortab.c \
	compute.c \
	condrender.c \
	conservativeraster.c \
	context.c \
	convolve.c \
	copyimage.c \
	cpuinfo.c \
	MESAdebug.c \
	debug_output.c \
	depth.c \
	dlist.c \
	draw.c \
	drawpix.c \
	drawtex.c \
	draw_validate.c \
	enable.c \
	enums.c \
	errors.c \
	MESAeval.c \
	execmem.c \
	extensions.c \
	extensions_table.c \
	externalobjects.c \
	fbobject.c \
	feedback.c \
	ff_fragment_shader.cpp \
	ffvertex_prog.c \
	fog.c \
	format_fallback.c \
	format_pack.c \
	format_unpack.c \
	format_utils.c \
	formatquery.c \
	formats.c \
	framebuffer.c \
	get.c \
	genmipmap.c \
	getstring.c \
	glformats.c \
	glspirv.c \
	glthread.c \
	hash.c \
	hint.c \
	histogram.c \
	image.c \
	imports.c \
	light.c \
	lines.c \
	marshal.c \
	marshal_generated.c \
	matrix.c \
	mipmap.c \
	mm.c \
	multisample.c \
	objectlabel.c \
	objectpurge.c \
	pack.c \
	pbo.c \
	performance_monitor.c \
	performance_query.c \
	pipelineobj.c \
	MESApixel.c \
	MESApixelstore.c \
	pixeltransfer.c \
	points.c \
	polygon.c \
	program_binary.c \
	program_resource.c \
	querymatrix.c \
	queryobj.c \
	rastpos.c \
	readpix.c \
	remap.c \
	renderbuffer.c \
	robustness.c \
	samplerobj.c \
	scissor.c \
	shaderapi.c \
	shaderimage.c \
	shaderobj.c \
	shader_query.cpp \
	shared.c \
	state.c \
	stencil.c \
	syncobj.c \
	texcompress.c \
	texcompress_astc.cpp \
	texcompress_bptc.c \
	texcompress_cpal.c \
	texcompress_etc.c \
	texcompress_fxt1.c \
	texcompress_rgtc.c \
	texcompress_s3tc.c \
	texenv.c \
	texformat.c \
	texgen.c \
	texgetimage.c \
	teximage.c \
	texobj.c \
	texparam.c \
	texstate.c \
	texstorage.c \
	texstore.c \
	texturebindless.c \
	textureview.c \
	transformfeedback.c \
	uniform_query.cpp \
	uniforms.c \
	varray.c \
	vdpau.c \
	version.c \
	viewport.c \
	vtxfmt.c \
	es1_conversion.c

# AMD common code
PATHS.amd=	amd/common amd/addrlib/src amd/addrlib/src/core \
		amd/addrlib/src/gfx9 amd/addrlib/src/r800
INCLUDES.amd=	amd amd/common ../../src/amd/common \
		amd/addrlib amd/addrlib/inc \
		amd/addrlib/src amd/addrlib/src/core \
		amd/addrlib/src/r800 \
		amd/addrlib/src/chip/r800 \
		amd/addrlib/src/gfx9 \
		amd/addrlib/src/chip/gfx9

SRCS.amd+= \
	addrinterface.cpp \
	addrelemlib.cpp \
	addrlib.cpp \
	addrlib1.cpp \
	addrlib2.cpp \
	addrobject.cpp \
	coord.cpp \
	gfx9addrlib.cpp \
	ciaddrlib.cpp \
	egbaddrlib.cpp \
	siaddrlib.cpp \
	ac_binary.c \
	ac_llvm_build.c \
	ac_llvm_helper.cpp \
	ac_llvm_util.c \
	ac_shader_util.c \
	ac_nir_to_llvm.c \
	ac_gpu_info.c \
	ac_surface.c \
	ac_debug.c

# XXX  avoid source name clashes with glx
.PATH:		${X11SRCDIR.Mesa}/src/mesa/main
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/mesa/main/pixel.c MESApixel.c \
		${X11SRCDIR.Mesa}/src/mesa/main/pixelstore.c MESApixelstore.c \
		${X11SRCDIR.Mesa}/src/mesa/main/eval.c MESAeval.c \
		${X11SRCDIR.Mesa}/src/mesa/main/debug.c MESAdebug.c

# Math sources
PATHS.math=	mesa/math
SRCS.math= \
	m_debug_clip.c \
	m_debug_norm.c \
	m_debug_xform.c \
	m_eval.c \
	m_matrix.c \
	m_translate.c \
	m_vector.c

PATHS.math_xform=	mesa/math
SRCS.math_xform= \
	m_xform.c


# VBO sources
PATHS.vbo=	mesa/vbo
INCLUDES.vbo=	gallium/auxiliary
SRCS.vbo= \
	vbo_context.c \
	vbo_exec_api.c \
	vbo_exec.c \
	vbo_exec_draw.c \
	vbo_exec_eval.c \
	vbo_minmax_index.c \
	vbo_noop.c \
	vbo_primitive_restart.c \
	vbo_save_api.c \
	vbo_save.c \
	vbo_save_draw.c \
	vbo_save_loopback.c

# TNL sources
PATHS.tnl=	mesa/tnl
SRCS.tnl= \
	t_context.c \
	t_draw.c \
	t_pipeline.c \
	t_rebase.c \
	t_split.c \
	t_split_copy.c \
	t_split_inplace.c \
	t_vb_fog.c \
	t_vb_light.c \
	t_vb_normals.c \
	t_vb_points.c \
	t_vb_program.c \
	t_vb_render.c \
	t_vb_texgen.c \
	t_vb_texmat.c \
	t_vb_vertex.c \
	t_vertex.c \
	t_vertex_generic.c \
	t_vertex_sse.c \
	t_vp_build.c

# Software raster sources
PATHS.swrast=		mesa/swrast
INCLUDES.swrast=	mesa/main
SRCS.swrast= \
	s_aaline.c \
	s_aatriangle.c \
	s_alpha.c \
	s_atifragshader.c \
	s_bitmap.c \
	s_blend.c \
	s_blit.c \
	s_clear.c \
	s_context.c \
	s_copypix.c \
	s_depth.c \
	s_drawpix.c \
	s_feedback.c \
	s_fog.c \
	s_fragprog.c \
	s_lines.c \
	s_logic.c \
	s_masking.c \
	s_points.c \
	s_renderbuffer.c \
	s_span.c \
	s_stencil.c \
	s_texcombine.c \
	s_texfetch.c \
	s_texfilter.c \
	s_texrender.c \
	s_texture.c \
	s_triangle.c \
	s_zoom.c

# swrast_setup
PATHS.ss=	mesa/swrast_setup
SRCS.ss= \
	ss_context.c \
	ss_triangle.c 


# Common driver sources
PATHS.common=	mesa/drivers/common
SRCS.common= \
	driverfuncs.c   \
	meta_blit.c     \
	meta_generate_mipmap.c  \
	meta.c

# ASM C driver sources
PATHS.asm_c=	mesa/x86 mesa/x86/rtasm mesa/sparc mesa/x86-64
SRCS.asm_c= \
	common_x86.c \
	x86_xform.c \
	3dnow.c \
	sse.c \
	x86sse.c \
	sparc.c \
	x86-64.c

# ASM assembler driver sources
PATHS.asm_s=	mesa/x86 mesa/x86/rtasm mesa/sparc mesa/x86-64
.if ${MACHINE} == "amd64"
SRCS.asm_s= \
	xform4.S
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa
.elif ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS.asm_s= \
	sparc_clip.S \
	norm.S \
	xform.S
.elif ${MACHINE} == "i386"
SRCS.asm_s= \
	common_x86_asm.S \
	x86_xform2.S \
	x86_xform3.S \
	x86_xform4.S \
	x86_cliptest.S \
	mmx_blend.S \
	3dnow_xform1.S \
	3dnow_xform2.S \
	3dnow_xform3.S \
	3dnow_xform4.S \
	sse_xform1.S \
	sse_xform2.S \
	sse_xform3.S \
	sse_xform4.S \
	sse_normal.S \
	read_rgba_span_x86.S \
	streaming-load-memcpy.c \
	sse_minmax.c
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa
.endif

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS.asm_s+= \
	streaming-load-memcpy.c \
	sse_minmax.c
COPTS.sse_minmax.c+= -msse4.1
.endif

# State tracker sources
PATHS.state_tracker=	mesa/state_tracker
INCLUDES.state_tracker=	glsl mesa/main
SRCS.state_tracker= \
	st_atifs_to_tgsi.c \
	st_atom.c \
	st_atom_array.c \
	st_atom_atomicbuf.c \
	st_atom_blend.c \
	st_atom_clip.c \
	st_atom_constbuf.c \
	st_atom_depth.c \
	st_atom_framebuffer.c \
	st_atom_image.c \
	st_atom_msaa.c \
	st_atom_pixeltransfer.c \
	st_atom_rasterizer.c \
	st_atom_sampler.c \
	st_atom_scissor.c \
	st_atom_shader.c \
	st_atom_stipple.c \
	st_atom_storagebuf.c \
	st_atom_tess.c \
	st_atom_texture.c \
	st_atom_viewport.c \
	st_cb_bitmap.c \
	st_cb_bitmap_shader.c \
	st_cb_blit.c \
	st_cb_bufferobjects.c \
	st_cb_clear.c \
	st_cb_compute.c \
	st_cb_condrender.c \
	st_cb_copyimage.c \
	st_cb_drawpixels.c \
	st_cb_drawpixels_shader.c \
	st_cb_drawtex.c \
	st_cb_eglimage.c \
	st_cb_fbo.c \
	st_cb_feedback.c \
	st_cb_flush.c \
	st_cb_memoryobjects.c \
	st_cb_msaa.c \
	st_cb_perfmon.c \
	st_cb_program.c \
	st_cb_queryobj.c \
	st_cb_rasterpos.c \
	st_cb_readpixels.c \
	st_cb_semaphoreobjects.c \
	st_cb_strings.c \
	st_cb_syncobj.c \
	st_cb_texture.c \
	st_cb_texturebarrier.c \
	st_cb_viewport.c \
	st_cb_xformfb.c \
	st_context.c \
	st_copytex.c \
	st_debug.c \
	st_draw.c \
	st_draw_feedback.c \
	st_extensions.c \
	st_format.c \
	st_gen_mipmap.c \
	st_glsl_to_ir.cpp \
	st_glsl_to_nir.cpp \
	st_glsl_to_tgsi.cpp \
	st_glsl_to_tgsi_array_merge.cpp \
	st_glsl_to_tgsi_private.cpp \
	st_glsl_to_tgsi_temprename.cpp \
	st_glsl_types.cpp \
	st_manager.c \
	st_mesa_to_tgsi.c \
	st_nir_builtins.c \
	st_nir_lower_builtin.c \
	st_nir_lower_tex_src_plane.c \
	st_pbo.c \
	st_program.c \
	st_sampler_view.c \
	st_scissor.c \
	st_shader_cache.c \
	st_texture.c \
	st_tgsi_lower_yuv.c

# Program sources
PATHS.program=	mesa/program ../../src/mesa/program
INCLUDES.program=	glsl
SRCS.program= \
	arbprogparse.c \
	ir_to_mesa.cpp \
	prog_cache.c \
	prog_execute.c \
	prog_instruction.c \
	prog_noise.c \
	prog_opt_constant_fold.c \
	prog_optimize.c \
	prog_parameter.c \
	prog_parameter_layout.c \
	prog_print.c \
	prog_statevars.c \
	prog_to_nir.c \
	program.c \
	programopt.c \
	program_parse.tab.c \
	program_parse_extra.c \
	symbol_table.c \
	program_lexer.l

# Generated
#.PATH:	${X11SRCDIR.Mesa}/../src/mesa/program
#SRCS.program+= \
#	lex.yy.c


# Run throught all the modules and setup the SRCS and CPPFLAGS etc.
.for _mod_ in ${MESA_SRC_MODULES}

SRCS+=	${SRCS.${_mod_}}

. for _path_ in ${PATHS.${_mod_}}
.PATH:	${X11SRCDIR.Mesa}/src/${_path_}
. endfor

. for _path_ in ${INCLUDES.${_mod_}}
.  for _s in ${SRCS.${_mod_}}
CPPFLAGS.${_s}+=	-I${X11SRCDIR.Mesa}/src/${_path_}
.  endfor
. endfor

.endfor

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mesa
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mapi
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/gallium/include
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mapi/glapi
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/../src/mesa/main
CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common

CPPFLAGS+=	\
	-DPACKAGE_NAME=\"Mesa\" \
	-DPACKAGE_TARNAME=\"mesa\" \
	-DPACKAGE_VERSION=\"${MESA_VER}\" \
	-DPACKAGE_STRING=\"Mesa\ ${MESA_VER}\" \
	-DVERSION=\"${MESA_VER}\" \
	-DPACKAGE_BUGREPORT=\"https://bugs.freedesktop.org/enter_bug.cgi\?product=Mesa\" \
	-DPACKAGE_URL=\"\" \
	-DPACKAGE=\"mesa\" \

CPPFLAGS+=	\
	-DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 \
	-DHAVE___BUILTIN_BSWAP32=1 -DHAVE___BUILTIN_BSWAP64=1 \
	-DHAVE___BUILTIN_CLZ=1 -DHAVE___BUILTIN_CLZLL=1 \
	-DHAVE___BUILTIN_CTZ=1 -DHAVE___BUILTIN_EXPECT=1 \
	-DHAVE___BUILTIN_FFS=1 -DHAVE___BUILTIN_FFSLL=1 \
	-DHAVE___BUILTIN_POPCOUNT=1 -DHAVE___BUILTIN_POPCOUNTLL=1 \
	-DHAVE___BUILTIN_UNREACHABLE=1 -DHAVE_FUNC_ATTRIBUTE_CONST=1 \
	-DHAVE_FUNC_ATTRIBUTE_FLATTEN=1 -DHAVE_FUNC_ATTRIBUTE_FORMAT=1 \
	-DHAVE_FUNC_ATTRIBUTE_MALLOC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 \
	-DHAVE_FUNC_ATTRIBUTE_PURE=1 -DHAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL=1 \
	-DHAVE_FUNC_ATTRIBUTE_UNUSED=1 -DHAVE_FUNC_ATTRIBUTE_VISIBILITY=1 \
	-DHAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT=1 \
	-DHAVE_FUNC_ATTRIBUTE_WEAK=1 -DHAVE_FUNC_ATTRIBUTE_ALIAS=1 \
	-DHAVE_FUNC_ATTRIBUTE_NORETURN=1 -DHAVE_ENDIAN_H=1 -DHAVE_DLADDR=1 \
	-DHAVE_CLOCK_GETTIME=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 \
	-DHAVE_PTHREAD=1 \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-DUSE_GCC_ATOMIC_BUILTINS \
	-DNDEBUG \
	-DHAVE_SYS_SYSCTL_H \
	-DHAVE_DLFCN_H \
	-DHAVE_STRTOF \
	-DHAVE_MKOSTEMP \
	-DHAVE_TIMESPEC_GET \
	-DHAVE_STRTOD_L \
	-DHAVE_DL_ITERATE_PHDR \
	-DHAVE_POSIX_MEMALIGN \
	-DHAVE_ZLIB \
	-DHAVE_LIBDRM -DGLX_USE_DRM \
	-DGLX_INDIRECT_RENDERING \
	-DGLX_DIRECT_RENDERING \
	-DGLX_USE_TLS \
	-DHAVE_X11_PLATFORM \
	-DHAVE_DRM_PLATFORM \
	-DENABLE_SHADER_CACHE \
	-DHAVE_MINCORE

.if ${MKLLVMRT} != "no"
LLVM_VERSION!=		cd ${NETBSDSRCDIR}/external/apache2/llvm && ${MAKE} -V LLVM_VERSION
HAVE_LLVM_VERSION!=	expr ${LLVM_VERSION:R:R} \* 256 + ${LLVM_VERSION:R:E} \* 16
CPPFLAGS+=	\
	-DHAVE_LLVM=${HAVE_LLVM_VERSION}
CXXFLAGS+=	-fno-rtti
.endif

.include "../asm.mk"

CPPFLAGS+=	\
	-DHAVE_LIBDRM -DGLX_USE_DRM -DGLX_INDIRECT_RENDERING -DGLX_DIRECT_RENDERING -DHAVE_ALIAS -DMESA_EGL_NO_X11_HEADERS

CPPFLAGS+=	\
	-DYYTEXT_POINTER=1

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp -fcommon

.include "libGL.old/mesa-ver.mk"
