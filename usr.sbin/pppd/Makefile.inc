#	$NetBSD: Makefile.inc,v 1.4.12.1 2008/09/18 04:30:10 wrstuden Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
