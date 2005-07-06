#	$NetBSD: Makefile,v 1.4.4.1 2005/07/06 20:12:40 snj Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

.include <bsd.lib.mk>
