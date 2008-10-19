# $NetBSD: Makefile,v 1.3.4.2 2008/10/19 22:41:06 haad Exp $

MODULE=	s3v

SRCS=	s3v_context.c s3v_dd.c s3v_inithw.c s3v_lock.c s3v_render.c \
        s3v_screen.c s3v_span.c s3v_state.c s3v_tex.c s3v_texmem.c \
        s3v_texstate.c s3v_tris.c s3v_vb.c s3v_xmesa.c

COPTS.s3v_tris.c=	-Wno-error
COPTS.s3v_texmem.c=	-Wno-error

.include "${.CURDIR}/../dri.mk"
