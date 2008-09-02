#	$NetBSD: Makefile,v 1.1 2008/09/02 08:38:36 christos Exp $

NOMAN=

.include "../Makefile.inc"

HOSTPROG=	makedoc

.PATH: ${IDIST}/info
HOST_CPPFLAGS+= -I${IDIST}/info -I${IDIST}/lib -I${.CURDIR}/../common

.include <bsd.hostprog.mk>
