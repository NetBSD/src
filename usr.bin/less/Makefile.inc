#	$NetBSD: Makefile.inc,v 1.5 2011/07/03 15:28:47 mrg Exp $

WARNS?=	2

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

CWARNFLAGS+=	-Wno-strict-prototypes -Wno-missing-prototypes -Wno-shadow
