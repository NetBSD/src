#	$NetBSD: Makefile.inc,v 1.13.6.1 2014/05/22 11:36:48 yamt Exp $

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S
SRCS+=		powerpc_initfini.c

CPPFLAGS+=	-I.

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
