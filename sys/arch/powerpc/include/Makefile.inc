# $NetBSD: Makefile.inc,v 1.1 2002/12/19 19:36:26 thorpej Exp $

.if !defined(_POWERPC_MAKEFILE_INC)
_POWERPC_MAKEFILE_INC=	yes

CFLAGS+= -msoft-float

.endif
