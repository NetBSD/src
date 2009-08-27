#	$NetBSD: Makefile.inc,v 1.8.34.2 2009/08/27 16:51:27 matt Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
CPPFLAGS+= -DSOFTFLOAT_NEED_FIXUNS
.endif
