#	$NetBSD: Makefile.inc,v 1.1 1997/10/17 13:32:28 lukem Exp $

WARNS?=	0

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
