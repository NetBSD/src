# $NetBSD: Makefile,v 1.4 2010/05/23 21:31:53 mrg Exp $

MODULE=	r300

CPPFLAGS+=	-DCOMPILE_R300 -DR200_MERGED=0 \
		-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R300 \
		-DRADEON_R300
CPPFLAGS+=	-DHAVE_LIBDRM_RADEON

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/r300 \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon/server

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon
SRCS=	\
	radeon_bo_legacy.c \
	radeon_buffer_objects.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_cs_legacy.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_lock.c \
	radeon_mipmap_tree.c \
	radeon_span.c \
	radeon_queryobj.c \
	radeon_texture.c \
	radeon_screen.c \
	r300_context.c \
	r300_draw.c \
	r300_ioctl.c \
	r300_cmdbuf.c \
	r300_state.c \
	r300_render.c \
	r300_tex.c \
	r300_texstate.c \
	r300_vertprog.c \
	r300_fragprog_common.c \
	r300_shader.c \
	radeon_mesa_to_rc.c \
	r300_emit.c \
	r300_swtcl.c

SRCS+= \
	radeon_cs_space_drm.c \
	radeon_bo.c \
	radeon_cs.c

#	../common/mm.c \
#	../common/vblank.c \
#	../common/xmlconfig.c \
#	../common/dri_util.c \

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
