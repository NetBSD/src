# $NetBSD: Makefile,v 1.2.6.1 2009/09/17 04:24:30 snj Exp $

MODULE=	r300

CPPFLAGS+=	-DCOMPILE_R300 -DR200_MERGED=0 \
		-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R300

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/r300 \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon/server

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon
SRCS=	\
	radeon_screen.c \
	radeon_context.c \
	radeon_ioctl.c \
	radeon_lock.c \
	radeon_span.c \
	radeon_state.c \
	r300_mem.c \
	r300_context.c \
	r300_ioctl.c \
	r300_cmdbuf.c \
	r300_state.c \
	r300_render.c \
	r300_texmem.c \
	r300_tex.c \
	r300_texstate.c \
	radeon_program.c \
	radeon_program_alu.c \
	radeon_program_pair.c \
	radeon_nqssadce.c \
	r300_vertprog.c \
	r300_fragprog.c \
	r300_fragprog_swizzle.c \
	r300_fragprog_emit.c \
	r500_fragprog.c \
	r500_fragprog_emit.c \
	r300_shader.c \
	r300_emit.c \
	r300_swtcl.c

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
