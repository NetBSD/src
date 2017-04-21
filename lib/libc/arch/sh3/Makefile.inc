#	$NetBSD: Makefile.inc,v 1.6.72.1 2017/04/21 16:53:08 bouyer Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if ${MKSOFTFLOAT} != "no"

CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>

.endif
