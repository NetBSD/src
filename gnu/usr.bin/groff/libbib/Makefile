#	$NetBSD: Makefile,v 1.11 2001/04/19 14:41:25 wiz Exp $

.include "../Makefile.comp"

DIST=		${IDIST}/src/libs/libbib
.PATH:		${DIST}

LIB=		bib
SRCS=		common.cc index.cc linear.cc search.cc map.c
MKMAN=		no
MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
