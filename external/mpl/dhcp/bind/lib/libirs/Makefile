#	$NetBSD: Makefile,v 1.1.2.2 2024/02/24 13:07:34 martin Exp $

LIBISPRIVATE=   yes
LIB=irs

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include

DIST=   ${IDIST}/lib/irs

.PATH.c:	${DIST}

SRCS=		context.c dnsconf.c resconf.c version.c


.include <bsd.lib.mk>
