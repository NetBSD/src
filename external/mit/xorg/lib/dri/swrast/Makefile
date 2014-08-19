# $NetBSD: Makefile,v 1.1.16.1 2014/08/20 00:00:11 tls Exp $

MODULE=	swrast

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast
SRCS= \
	swrast.c \
        swrast_span.c

.include "${.CURDIR}/../dri.mk"
