# $NetBSD: Makefile,v 1.6 2010/05/23 23:21:30 mrg Exp $

MODULE=	r200

CPPFLAGS+=	-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R200 -DRADEON_R200
CPPFLAGS+=	-DHAVE_LIBDRM_RADEON

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/r200 \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon/server

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon
SRCS=	\
	radeon_bo_legacy.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_cs_legacy.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_lock.c \
	radeon_mipmap_tree.c \
	radeon_queryobj.c \
	radeon_span.c \
	radeon_texture.c \
	r200_context.c \
	r200_ioctl.c \
	r200_state.c \
	r200_state_init.c \
	r200_cmdbuf.c \
	r200_pixel.c \
	r200_tex.c \
	r200_texstate.c \
	r200_tcl.c \
	r200_swtcl.c \
	r200_maos.c \
	r200_sanity.c \
	r200_fragshader.c \
	r200_vertprog.c \
	radeon_screen.c

SRCS+= \
	radeon_cs_space_drm.c \
	radeon_bo.c \
	radeon_cs.c

LIBDPLIBS+=	drm_radeon	${.CURDIR}/../../libdrm_radeon

# XXX
.if ${MACHINE_ARCH} == "sparc64"
COPTS.r200_tcl.c=	-Wno-error
.endif

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
