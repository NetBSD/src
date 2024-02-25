#	$NetBSD: Makefile,v 1.2.6.1 2024/02/25 15:47:57 martin Exp $

LIB=irs
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include

DIST=   ${IDIST}/lib/irs

.PATH.c:	${DIST}

SRCS=		resconf.c


.include <bsd.lib.mk>
