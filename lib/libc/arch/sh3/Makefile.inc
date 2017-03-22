#	$NetBSD: Makefile.inc,v 1.7 2017/03/22 23:11:08 chs Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if ${MKSOFTFLOAT} != "no"

CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>

.endif
