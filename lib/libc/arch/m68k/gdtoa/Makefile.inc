#	$NetBSD: Makefile.inc,v 1.3.56.1 2013/07/23 21:07:28 riastradh Exp $

SRCS+=	strtof.c

.if ${MACHINE_ARCH} == "m68k"
SRCS+=	strtold_pxL.c
SRCS+=	strtopxL.c
.endif
