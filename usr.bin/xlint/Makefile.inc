#	$NetBSD: Makefile.inc,v 1.4 2001/10/22 05:19:31 jmc Exp $

CPPFLAGS+=-I${.CURDIR}/../lint1/arch/${MACHINE_CPU}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
