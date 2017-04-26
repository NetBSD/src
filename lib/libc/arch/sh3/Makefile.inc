#	$NetBSD: Makefile.inc,v 1.6.68.1 2017/04/26 02:52:54 pgoyette Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

.if ${MKSOFTFLOAT} != "no"

CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>

.endif
