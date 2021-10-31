#	$NetBSD: Makefile.inc,v 1.15 2021/10/31 22:24:35 thorpej Exp $

SRCS+=		__sigtramp2.S
SRCS+=		powerpc_initfini.c

CPPFLAGS+=	-I.

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
