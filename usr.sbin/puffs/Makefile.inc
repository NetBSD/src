#	$NetBSD: Makefile.inc,v 1.2 2007/05/28 12:06:39 tls Exp $

USE_FORT?= yes	# network client

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
