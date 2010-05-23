# $NetBSD: Makefile,v 1.1 2010/05/23 21:31:53 mrg Exp $

MODULE=	r600

CPPFLAGS+=	-DRADEON_COMMON=1 -DRADEON_R600
CPPFLAGS+=	-DHAVE_LIBDRM_RADEON

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/r600 \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon/server

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon
SRCS=	\
	radeon_bo_legacy.c \
	radeon_common_context.c \
	radeon_buffer_objects.c \
	radeon_common.c \
	radeon_cs_legacy.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_lock.c \
	radeon_mipmap_tree.c \
	radeon_span.c \
	radeon_texture.c \
	radeon_queryobj.c \
	radeon_screen.c \
	r600_context.c \
	r600_cmdbuf.c \
	r600_emit.c       \
	r700_assembler.c  \
	r700_fragprog.c \
	r700_vertprog.c \
	r700_shader.c \
	r700_shaderinst.c \
	r700_ioctl.c \
	r700_oglprog.c \
	r700_chip.c     \
	r700_state.c    \
	r700_clear.c    \
	r700_render.c   \
	r600_tex.c      \
	r600_texstate.c      \
	r700_debug.c

SRCS+= \
	radeon_cs_space_drm.c \
	radeon_bo.c \
	radeon_cs.c

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
