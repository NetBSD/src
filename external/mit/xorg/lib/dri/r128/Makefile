# $NetBSD: Makefile,v 1.5 2014/07/09 20:22:55 riastradh Exp $

MODULE=	r128

SRCS=	r128_context.c r128_lock.c r128_state.c r128_texstate.c r128_dd.c \
	r128_screen.c r128_tex.c r128_tris.c r128_ioctl.c r128_span.c \
	r128_texmem.c

CWARNFLAGS.clang+=	-Wno-constant-logical-operand

.include "${.CURDIR}/../dri.mk"
