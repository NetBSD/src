#	$NetBSD: Makefile,v 1.2 2008/09/02 14:46:53 christos Exp $

NOMAN=
MAKEDOC=

.include "../Makefile.inc"

HOSTPROG=	makedoc

.PATH: ${IDIST}/info
HOST_CPPFLAGS+= -I${IDIST}/info -I${IDIST}/lib -I${.CURDIR}/../common
HOST_CPPFLAGS+= -Dxstrdup=strdup -Dxmalloc=malloc -Dxfree=free
HOST_CPPFLAGS+= -Dxexit=exit -Dxrealloc=realloc

LDADD+=-lintl
DPADD+=${LIBINTL}

.include <bsd.hostprog.mk>
