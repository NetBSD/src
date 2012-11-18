# $NetBSD: Makefile.inc,v 1.1.4.2 2012/11/18 22:38:32 riz Exp $

WARNS=		5
MKLINT=		no

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
