#	$NetBSD: Makefile.inc,v 1.5 2001/10/25 02:22:55 thorpej Exp $

.if !defined(HOSTPROG)
CPPFLAGS+=-I${.CURDIR}/../lint1/arch/${MACHINE_CPU}
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
