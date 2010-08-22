# $NetBSD: Makefile.inc,v 1.1 2010/08/22 18:56:23 rmind Exp $

WARNS?=		4

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
