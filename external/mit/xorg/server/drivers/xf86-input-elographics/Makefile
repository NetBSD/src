#	$NetBSD: Makefile,v 1.2 2015/11/06 21:32:22 szptvlfn Exp $

DRIVER=		xf86-input-elographics
DRIVER_NAME=	elographics_drv

SRCS=		xf86Elo.c
MAN=		elographics.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

COPTS.xf86Elo.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
