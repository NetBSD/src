#	$NetBSD: Makefile,v 1.3 2008/09/03 09:55:23 dogcow Exp $

NOMAN=
MAKEDOC=

.include "../Makefile.inc"

HOSTPROG=	makedoc

.PATH: ${IDIST}/info
HOST_CPPFLAGS+= -I${IDIST}/info -I${IDIST}/lib -I${.CURDIR}/../common
HOST_CPPFLAGS+= -Dxstrdup=strdup -Dxmalloc=malloc -Dxfree=free
HOST_CPPFLAGS+= -Dxexit=exit -Dxrealloc=realloc -DHOSTTOOL

.include <bsd.hostprog.mk>
