# $NetBSD: Makefile.inc,v 1.1.2.2 2002/12/19 23:20:18 thorpej Exp $

.if !defined(_POWERPC_MAKEFILE_INC)
_POWERPC_MAKEFILE_INC=	yes

CFLAGS+= -msoft-float

.endif
