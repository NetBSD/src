#	$NetBSD: Makefile.inc,v 1.1 1997/10/20 04:28:24 lukem Exp $

WARNS=	0

.if exists("${.CURDIR}/../../Makefile.inc")
.include "${.CURDIR}/../../Makefile.inc"
.endif
