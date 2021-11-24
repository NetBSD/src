#	$NetBSD: Makefile.inc,v 1.9 2021/11/24 02:01:15 thorpej Exp $

SRCS+=	__sigtramp2.S

CPPFLAGS+= -I.

.if ${MKSOFTFLOAT} != "no"

CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS
.include <softfloat/Makefile.inc>

.endif
