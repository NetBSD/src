#	$NetBSD: Makefile.inc,v 1.1.2.1 1997/10/27 19:20:19 mellon Exp $

WARNS=	0

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
