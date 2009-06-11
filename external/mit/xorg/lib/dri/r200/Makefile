# $NetBSD: Makefile,v 1.4 2009/06/11 01:47:08 mrg Exp $

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

# XXX
.if ${MACHINE_ARCH} == "sparc64"
COPTS.r200_tcl.c=	-Wno-error
.endif

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
