#	$NetBSD: Makefile,v 1.3 2024/02/21 22:52:54 christos Exp $

LIB=irs
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include

DIST=   ${IDIST}/lib/irs

.PATH.c:	${DIST}

SRCS=		resconf.c


.include <bsd.lib.mk>
