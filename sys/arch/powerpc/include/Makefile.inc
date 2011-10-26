# $NetBSD: Makefile.inc,v 1.2 2011/10/26 15:57:26 chs Exp $

.if !defined(_POWERPC_MAKEFILE_INC)
_POWERPC_MAKEFILE_INC=	yes

.if !defined(RUMPKERNEL)
CFLAGS+= -msoft-float
.endif

.endif
