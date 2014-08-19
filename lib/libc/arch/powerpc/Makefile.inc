#	$NetBSD: Makefile.inc,v 1.13.12.1 2014/08/20 00:02:11 tls Exp $

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S
SRCS+=		powerpc_initfini.c

CPPFLAGS+=	-I.

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
