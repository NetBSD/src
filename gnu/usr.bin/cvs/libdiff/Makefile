#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:03 tv Exp $

.include "${.CURDIR}/../Makefile.inc"

LIB=	diff

DIST=	${IDIST}/${LIB}
.PATH:	${DIST}

SRCS !=	echo ${DIST}/*.c | sed -e s,${DIST}/,,g

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
