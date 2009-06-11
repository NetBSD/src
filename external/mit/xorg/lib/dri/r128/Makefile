# $NetBSD: Makefile,v 1.3 2009/06/11 01:47:08 mrg Exp $

MODULE=	r128

SRCS=	r128_context.c r128_lock.c r128_state.c r128_texstate.c r128_dd.c \
	r128_screen.c r128_tex.c r128_tris.c r128_ioctl.c r128_span.c \
	r128_texmem.c

COPTS.r128_span.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
