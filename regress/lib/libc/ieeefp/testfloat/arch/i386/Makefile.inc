# $NetBSD: Makefile.inc,v 1.2 2001/03/22 12:05:36 ross Exp $

CFLAGS+=	-ffloat-store -DFLOATX80 ## -DLONG_DOUBLE_IS_FLOATX80

SRCS+=		systfloat.S systmodes.S
