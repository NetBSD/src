# $NetBSD: Makefile,v 1.2.4.2 2008/10/19 22:41:07 haad Exp $

MODULE=	unichrome

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/unichrome

SRCS=	via_context.c via_fb.c via_tex.c via_ioctl.c via_memcpy.c \
	via_render.c via_screen.c via_span.c via_state.c via_texcombine.c \
	via_tris.c 

.include "${.CURDIR}/../dri.mk"
