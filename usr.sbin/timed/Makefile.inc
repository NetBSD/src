#	$NetBSD: Makefile.inc,v 1.4 2009/04/22 15:23:09 lukem Exp $

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

USE_FORT?=yes	# network client/server
