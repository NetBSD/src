# $NetBSD: Makefile,v 1.1 2008/09/21 00:47:00 cube Exp $

LIB=	s3v

SRCS=	s3v_context.c s3v_dd.c s3v_inithw.c s3v_lock.c s3v_render.c \
        s3v_screen.c s3v_span.c s3v_state.c s3v_tex.c s3v_texmem.c \
        s3v_texstate.c s3v_tris.c s3v_vb.c s3v_xmesa.c

COPTS.s3v_tris.c=	-Wno-error

.include "${.CURDIR}/../dri.mk"
