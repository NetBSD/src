#       $NetBSD: Makefile,v 1.2.8.2 2014/12/25 17:54:37 msaitoh Exp $

LIB=irs
#USE_SHLIBDIR=	yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+=	-I${DIST}/include -I${IDIST}/lib/irs/include

DIST=	${IDIST}/lib/irs
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	context.c dnsconf.c resconf.c version.c


.include <bsd.lib.mk>

