#	$NetBSD: Makefile.inc,v 1.2 2002/07/04 10:53:26 fvdl Exp $

.if (${MACHINE_ARCH} == "x86_64")
SRCS+=	x86_64_mtrr.c x86_64_iopl.c
.endif

MAN+=	x86_64_get_mtrr.2 x86_64_iopl.2

MLINKS+=x86_64_get_mtrr.2 x86_64_set_mtrr.2
