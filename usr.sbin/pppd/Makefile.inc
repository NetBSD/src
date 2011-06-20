#	$NetBSD: Makefile.inc,v 1.7 2011/06/20 07:44:03 mrg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC)
COPTS+=	-fno-strict-aliasing
.endif
