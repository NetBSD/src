#	$NetBSD: Makefile,v 1.4 2001/12/12 23:46:45 lukem Exp $

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
