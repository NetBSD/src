# $NetBSD: Makefile.inc,v 1.3 2014/08/10 17:44:26 joerg Exp $

.if !defined(_POWERPC_MAKEFILE_INC)
_POWERPC_MAKEFILE_INC=	yes

.if !defined(RUMPKERNEL)
CFLAGS+= ${${ACTIVE_CC} == "gcc":? -msoft-float :}
.endif

.endif
