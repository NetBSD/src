#	$NetBSD: Makefile,v 1.13 2002/09/19 03:09:40 lukem Exp $

NOMAN=		# defined
NOPROFILE=	# defined
NOPIC=		# defined

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libbib
.PATH:		${DIST}

LIB=		bib
SRCS=		common.cc index.cc linear.cc search.cc map.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
