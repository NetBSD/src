#	$NetBSD: Makefile,v 1.5 2004/05/23 02:24:50 lukem Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

.include <bsd.lib.mk>
