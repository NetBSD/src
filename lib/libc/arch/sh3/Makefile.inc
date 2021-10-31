#	$NetBSD: Makefile.inc,v 1.8 2021/10/31 22:24:35 thorpej Exp $

SRCS+=	__sigtramp2.S

.if ${MKSOFTFLOAT} != "no"

CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>

.endif
