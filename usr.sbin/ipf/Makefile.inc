#	$NetBSD: Makefile.inc,v 1.4.2.1 1997/10/30 07:15:57 mrg Exp $

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
