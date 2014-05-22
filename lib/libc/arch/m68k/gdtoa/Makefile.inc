#	$NetBSD: Makefile.inc,v 1.3.44.1 2014/05/22 11:36:47 yamt Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH} == "m68k"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.endif
