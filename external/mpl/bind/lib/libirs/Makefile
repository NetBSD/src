#	$NetBSD: Makefile,v 1.2 2021/02/19 16:42:24 christos Exp $

LIB=irs
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include

DIST=   ${IDIST}/lib/irs

.PATH.c:	${DIST}

SRCS=		context.c dnsconf.c resconf.c version.c


.include <bsd.lib.mk>
