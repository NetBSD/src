# $NetBSD: Makefile,v 1.2.6.1 2009/09/17 04:24:30 snj Exp $

MODULE=	tdfx

SRCS=	tdfx_context.c tdfx_dd.c tdfx_lock.c tdfx_pixels.c tdfx_render.c \
	tdfx_screen.c tdfx_span.c tdfx_state.c tdfx_tex.c tdfx_texman.c \
	tdfx_texstate.c tdfx_tris.c tdfx_vb.c

COPTS.tdfx_screen.c=  -Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
