#	$NetBSD: Makefile.inc,v 1.7 2005/05/03 04:37:33 matt Exp $

.if ${OBJECT_FMT} == "ELF"
SRCS+=	__longjmp14.c
.endif
SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S __sigtramp3.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__
