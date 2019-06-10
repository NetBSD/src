#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:04:53 christos Exp $

LIB=irs
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include

DIST=   ${IDIST}/lib/irs
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=		context.c dnsconf.c resconf.c version.c


.include <bsd.lib.mk>
