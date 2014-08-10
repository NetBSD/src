# $NetBSD: Makefile,v 1.4.18.1 2014/08/10 06:50:56 tls Exp $

MODULE=	tdfx

SRCS=	tdfx_context.c tdfx_dd.c tdfx_lock.c tdfx_pixels.c tdfx_render.c \
	tdfx_screen.c tdfx_span.c tdfx_state.c tdfx_tex.c tdfx_texman.c \
	tdfx_texstate.c tdfx_tris.c tdfx_vb.c

CWARNFLAGS.clang+=	-Wno-constant-logical-operand \
			-Wno-tautological-compare

.include "${.CURDIR}/../dri.mk"
