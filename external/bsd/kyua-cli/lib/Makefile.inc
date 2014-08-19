# $NetBSD: Makefile.inc,v 1.1.4.3 2014/08/19 23:46:49 tls Exp $

.include "../Makefile.inc"

LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     lua	${.CURDIR}/../../../../mit/lua/lib/liblua
LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm
