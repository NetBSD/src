# $NetBSD: Makefile.inc,v 1.1.2.2 2012/10/30 18:59:19 yamt Exp $

WARNS=		5
MKLINT=		no

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
