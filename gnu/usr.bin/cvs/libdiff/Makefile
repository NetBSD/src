#	$NetBSD: Makefile,v 1.2 2000/09/04 22:14:32 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
