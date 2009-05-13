#	$NetBSD: Makefile.inc,v 1.3.20.1 2009/05/13 19:20:42 jym Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server
