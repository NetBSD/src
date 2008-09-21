# $NetBSD: Makefile,v 1.1 2008/09/21 00:47:00 cube Exp $

LIB=	radeon

CPPFLAGS+=	-DRADEON_COMMON=0

SRCS=	radeon_context.c radeon_ioctl.c radeon_lock.c radeon_screen.c \
	radeon_state.c radeon_state_init.c radeon_tex.c radeon_texmem.c \
	radeon_texstate.c radeon_tcl.c radeon_swtcl.c radeon_span.c \
	radeon_maos.c radeon_sanity.c

.include "${.CURDIR}/../dri.mk"
