#	$NetBSD: Makefile,v 1.4.6.2 2005/07/07 14:18:03 snj Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

# only needed during build
libinstall::

.include <bsd.lib.mk>
