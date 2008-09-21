# $NetBSD: Makefile,v 1.2 2008/09/21 14:08:46 lukem Exp $

MODULE=	r200

CPPFLAGS+=	-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R200

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/r200 \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon \
		-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon/server

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/radeon
SRCS=	r200_context.c r200_ioctl.c r200_lock.c r200_state.c \
	r200_state_init.c r200_cmdbuf.c r200_pixel.c r200_tex.c r200_texmem.c \
	r200_texstate.c r200_tcl.c r200_swtcl.c r200_span.c r200_maos.c \
	r200_sanity.c r200_fragshader.c r200_vertprog.c radeon_screen.c

.include "${.CURDIR}/../dri.mk"
