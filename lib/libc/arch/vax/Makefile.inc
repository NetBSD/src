#	$NetBSD: Makefile.inc,v 1.5 2004/03/21 18:19:52 matt Exp $

.if ${OBJECT_FMT} == "ELF"
SRCS+=	__longjmp14.c
.endif
SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S __sigtramp3.S
