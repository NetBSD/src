#	$NetBSD: Makefile.inc,v 1.1 2002/06/18 08:29:35 fvdl Exp $

.if (${MACHINE_ARCH} == "x86_64")
SRCS+=	x86_64_mtrr.c
.endif

MAN+=	x86_64_get_mtrr.2

MLINKS+=x86_64_get_mtrr.2 x86_64_set_mtrr.2
