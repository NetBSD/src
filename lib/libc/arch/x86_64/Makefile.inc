#	$NetBSD: Makefile.inc,v 1.7 2020/10/12 17:55:53 kamil Exp $

.if ${RUMPRUN} != "yes"
SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S
.endif

CPPFLAGS+=	-I.
