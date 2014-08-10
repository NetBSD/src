# $NetBSD: Makefile,v 1.4.18.1 2014/08/10 06:50:56 tls Exp $

MODULE=	r128

SRCS=	r128_context.c r128_lock.c r128_state.c r128_texstate.c r128_dd.c \
	r128_screen.c r128_tex.c r128_tris.c r128_ioctl.c r128_span.c \
	r128_texmem.c

CWARNFLAGS.clang+=	-Wno-constant-logical-operand

.include "${.CURDIR}/../dri.mk"
