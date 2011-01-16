#	$NetBSD: Makefile.inc,v 1.12 2011/01/16 02:43:10 matt Exp $

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+=	-I. -D_NOREGNAMES

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
.include <softfloat/Makefile.inc>
.endif
