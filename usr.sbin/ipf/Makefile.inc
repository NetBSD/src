#	$NetBSD: Makefile.inc,v 1.1.1.6 1998/07/12 14:47:59 veego Exp $

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
