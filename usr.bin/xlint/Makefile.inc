#	$NetBSD: Makefile.inc,v 1.6 2001/11/12 23:16:36 tv Exp $

CPPFLAGS+=	-I${.CURDIR}/../lint1/arch/${MACHINE_CPU}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
