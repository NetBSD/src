# $NetBSD: Makefile.inc,v 1.1.4.2 2013/02/25 00:26:20 tls Exp $

.include "../Makefile.inc"

LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     lua	${.CURDIR}/../../../../mit/lua/lib/liblua
.if ${HAVE_GCC} == 4
LIBDPLIBS+=     stdc++	${.CURDIR}/../../../../../gnu/lib/libstdc++-v3_4
.else
LIBDPLIBS+=     stdc++	${.CURDIR}/../../../../../external/gpl3/gcc/lib/libstdc++-v3
.endif
LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm
