#	$NetBSD: Makefile.inc,v 1.7 2002/01/18 20:39:17 thorpej Exp $

.PATH:		${.CURDIR}/../common

CPPFLAGS+=	-I${.CURDIR}/../arch/${MACHINE_CPU}
CPPFLAGS+=	-I${.CURDIR}/../common

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
