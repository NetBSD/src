#	$NetBSD: Makefile.inc,v 1.1.1.5 1998/05/17 16:29:41 veego Exp $

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
