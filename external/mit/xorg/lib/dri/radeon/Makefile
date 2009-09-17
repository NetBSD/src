# $NetBSD: Makefile,v 1.3.4.1 2009/09/17 04:24:30 snj Exp $

MODULE=	radeon

CPPFLAGS+=	-DRADEON_COMMON=0

SRCS=	radeon_context.c radeon_ioctl.c radeon_lock.c radeon_screen.c \
	radeon_state.c radeon_state_init.c radeon_tex.c radeon_texmem.c \
	radeon_texstate.c radeon_tcl.c radeon_swtcl.c radeon_span.c \
	radeon_maos.c radeon_sanity.c

# XXX
.if ${MACHINE_ARCH} == "sparc64"
COPTS.radeon_tcl.c=	-Wno-error
.endif

COPTS.radeon_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
