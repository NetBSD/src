# $NetBSD: Makefile.inc,v 1.1 2001/03/13 06:39:30 ross Exp $

CFLAGS+=	-ffloat-store -DFLOATX80 ## -DLONG_DOUBLE_IS_FLOATX80

SRCS+=		systmodes.S systflags.S systfloat.S
