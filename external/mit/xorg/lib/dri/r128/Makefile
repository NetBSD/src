# $NetBSD: Makefile,v 1.2.12.1 2010/04/21 05:24:34 matt Exp $

MODULE=	r128

SRCS=	r128_context.c r128_lock.c r128_state.c r128_texstate.c r128_dd.c \
	r128_screen.c r128_tex.c r128_tris.c r128_ioctl.c r128_span.c \
	r128_texmem.c

COPTS.r128_span.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
