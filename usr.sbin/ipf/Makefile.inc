#	$NetBSD: Makefile.inc,v 1.1.1.7 1998/11/22 14:21:43 mrg Exp $

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
