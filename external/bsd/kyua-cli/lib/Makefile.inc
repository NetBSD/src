# $NetBSD: Makefile.inc,v 1.2.4.2 2014/05/22 15:45:30 yamt Exp $

.include "../Makefile.inc"

LIBISCXX=	yes
LIBISPRIVATE=	yes

LIBDPLIBS+=     lua	${.CURDIR}/../../../../mit/lua/lib/liblua
LIBDPLIBS+=     m	${.CURDIR}/../../../../../lib/libm
