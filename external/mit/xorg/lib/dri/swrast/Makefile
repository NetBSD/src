# $NetBSD: Makefile,v 1.2 2014/07/09 20:22:55 riastradh Exp $

MODULE=	swrast

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast
SRCS= \
	swrast.c \
        swrast_span.c

.include "${.CURDIR}/../dri.mk"
