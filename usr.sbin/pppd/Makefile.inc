#	$NetBSD: Makefile.inc,v 1.7.2.1 2012/10/30 19:00:50 yamt Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif
