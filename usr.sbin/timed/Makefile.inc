#	$NetBSD: Makefile.inc,v 1.3 2007/05/28 12:06:42 tls Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server

WARNS=4
