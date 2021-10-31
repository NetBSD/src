#	$NetBSD: Makefile.inc,v 1.2 2021/10/31 22:24:35 thorpej Exp $

SRCS+=		__sigtramp2.S

CPPFLAGS+=	-I.

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
.endif
