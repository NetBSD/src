#	$NetBSD: Makefile.inc,v 1.22 2020/10/12 23:56:08 kamil Exp $

.if ${RUMPRUN} != "yes"
SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S
.endif

CPPFLAGS+=	-I.
