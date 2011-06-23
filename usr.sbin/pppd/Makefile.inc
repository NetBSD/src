#	$NetBSD: Makefile.inc,v 1.6.4.1 2011/06/23 14:20:49 cherry Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC)
COPTS+=	-fno-strict-aliasing
.endif
