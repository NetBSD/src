#	$NetBSD: Makefile.inc,v 1.1 1997/10/19 03:52:29 lukem Exp $

WARNS=	0

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
