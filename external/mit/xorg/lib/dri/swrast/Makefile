# $NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:35 matt Exp $

MODULE=	swrast

CPPFLAGS+=	-I${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast

DRI_EXTRA_PATHS=	${X11SRCDIR.MesaLib}/src/mesa/drivers/dri/swrast
SRCS= \
	swrast.c \
        swrast_span.c

COPTS.swrast.c=		-Wno-error	# XXX const

.include "${.CURDIR}/../dri.mk"
