#	$NetBSD: Makefile.inc,v 1.5.6.1 2009/05/13 19:20:33 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
