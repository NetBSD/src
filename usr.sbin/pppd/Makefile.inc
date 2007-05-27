#	$NetBSD: Makefile.inc,v 1.4 2007/05/27 18:11:38 tls Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
