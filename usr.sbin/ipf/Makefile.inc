#	$NetBSD: Makefile.inc,v 1.5 1997/10/30 16:09:57 mrg Exp $

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
