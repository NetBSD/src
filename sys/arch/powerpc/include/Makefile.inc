# $NetBSD: Makefile.inc,v 1.2.12.1 2014/08/20 00:03:19 tls Exp $

.if !defined(_POWERPC_MAKEFILE_INC)
_POWERPC_MAKEFILE_INC=	yes

.if !defined(RUMPKERNEL)
CFLAGS+= ${${ACTIVE_CC} == "gcc":? -msoft-float :}
.endif

.endif
