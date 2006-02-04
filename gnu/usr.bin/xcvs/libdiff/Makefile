#	$NetBSD: Makefile,v 1.1 2006/02/04 16:22:53 christos Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

.include <bsd.lib.mk>
