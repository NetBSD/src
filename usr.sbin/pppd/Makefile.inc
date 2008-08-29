#	$NetBSD: Makefile.inc,v 1.5 2008/08/29 00:02:25 gmcgarry Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
