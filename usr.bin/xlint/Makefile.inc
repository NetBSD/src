#	$NetBSD: Makefile.inc,v 1.2 1997/10/24 12:15:01 veego Exp $

WARNS=	0

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
