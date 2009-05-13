#	$NetBSD: Makefile,v 1.14.26.1 2009/05/13 19:20:49 jym Exp $

.include <bsd.own.mk>

LIB=		GL

# glx
.PATH:		${X11SRCDIR.xc}/lib/GL/glx
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/mesa/drivers/dri/common
SRCS.glx=	clientattrib.c compsize.c dispatch.c eval.c g_render.c \
		g_single.c g_vendpriv.c glthread.c glxcmds.c glxext.c \
		glxextensions.c indirect_init.c \
		pixel.c pixelstore.c render2.c renderpix.c single2.c \
		singlepix.c vertarr.c xfont.c glx_pbuffer.c glx_query.c \
		glx_texture_compression.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/drivers/dri/common
SRCS.glx+=	glcontextmodes.c

.PATH:	${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi
SRCS.glx+=	glapi.c

.if ${MACHINE_ARCH} == "i386"
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/mesa/x86
SRCS.glx+=	glapi_x86.S
CPPFLAGS+=	-DUSE_X86_ASM -I${X11SRCDIR.xc}/extras/Mesa/src/X86
# -DUSE_X86_ASM $(MMX_DEFS) $(3DNOW_DEFS) $(SSE_DEFS)
.endif


.if ${X11DRI} != "no"
# dri
.PATH:		${X11SRCDIR.xc}/lib/GL/dri
SRCS.dri=	XF86dri.c dri_glx.c
.for fn in ${SRCS.dri}
CPPFLAGS.${fn}=-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules\"
.endfor


# drm
.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/linux/drm
SRCS.drm=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c


# mesa
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src
SRCS.mesa=	accum.c api_arrayelt.c api_eval.c api_loopback.c \
		api_noop.c api_validate.c attrib.c blend.c buffers.c \
		clip.c colortab.c context.c convolve.c debug.c depth.c \
		dispatch.c dlist.c drawpix.c enable.c enums.c \
		MESAeval.c extensions.c feedback.c fog.c get.c \
		MESApixel.c glthread.c hash.c hint.c histogram.c image.c \
		imports.c light.c lines.c matrix.c mmath.c \
		points.c polygon.c rastpos.c state.c stencil.c \
		texcompress.c texformat.c teximage.c texobj.c texstate.c \
		texstore.c texutil.c varray.c vtxfmt.c vpexec.c \
		vpparse.c  vpstate.c 

# XXX  avoid source name clashes with glx
BUILDSYMLINKS=	${X11SRCDIR.xc}/extras/Mesa/src/pixel.c MESApixel.c \
		${X11SRCDIR.xc}/extras/Mesa/src/eval.c MESAeval.c


# array_cache
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/array_cache
SRCS.ac=	ac_context.c ac_import.c


# math
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/math
SRCS.math=	m_debug_clip.c m_debug_norm.c m_debug_xform.c \
		m_eval.c m_matrix.c m_translate.c m_vector.c m_xform.c


# swrast
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast
SRCS.swrast=	s_aaline.c s_aatriangle.c s_accum.c s_alpha.c \
		s_alphabuf.c s_bitmap.c s_blend.c s_buffers.c \
		s_context.c s_copypix.c s_depth.c s_drawpix.c \
		s_feedback.c s_fog.c s_histogram.c s_imaging.c \
		s_lines.c s_logic.c s_masking.c s_pixeltex.c \
		s_points.c s_readpix.c s_span.c s_stencil.c \
		s_texstore.c s_texture.c s_triangle.c s_zoom.c


# swrast_setup
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/swrast_setup
SRCS.ss=	ss_context.c ss_triangle.c ss_vb.c


# tnl
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src/tnl
SRCS.tnl=	t_array_api.c t_array_import.c t_context.c t_eval_api.c \
		t_imm_alloc.c t_imm_api.c t_imm_debug.c t_imm_dlist.c \
		t_imm_elt.c t_imm_eval.c t_imm_exec.c t_imm_fixup.c \
		t_pipeline.c t_vb_fog.c t_vb_light.c t_vb_normals.c \
		t_vb_points.c t_vb_program.c t_vb_render.c t_vb_texgen.c \
		t_vb_texmat.c t_vb_vertex.c


.for fn in ${SRCS.glx} ${SRCS.dri} ${SRCS.drm} ${SRCS.mesa} ${SRCS.ac} \
	   ${SRCS.math} ${SRCS.swrast} ${SRCS.ss} ${SRCS.tnl}
CPPFLAGS.${fn}+=-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/glx \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared/drm/kernel \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/extras/Mesa/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi
CPPFLAGS.${fn}+=${X11FLAGS.DRI}

.endfor

SRCS+=	${SRCS.dri} ${SRCS.drm} ${SRCS.mesa} ${SRCS.ac} ${SRCS.math} \
	${SRCS.swrast} ${SRCS.ss} ${SRCS.tnl}
.endif

SRCS+=	${SRCS.glx}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/glapi \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/main \
		-I${X11SRCDIR.xc}/extras/Mesa/src/mesa/drivers/dri/common \
		${X11FLAGS.THREADS} ${X11FLAGS.EXTENSION} -DGLXEXT \
		-DGLX_USE_MESA

LIBDPLIBS=\
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
