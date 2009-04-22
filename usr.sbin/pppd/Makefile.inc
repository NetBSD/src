#	$NetBSD: Makefile.inc,v 1.6 2009/04/22 15:23:06 lukem Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
