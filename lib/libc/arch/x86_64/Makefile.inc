#	$NetBSD: Makefile.inc,v 1.8 2021/10/31 22:24:36 thorpej Exp $

.if ${RUMPRUN} != "yes"
SRCS+=	__sigtramp2.S
.endif

CPPFLAGS+=	-I.
