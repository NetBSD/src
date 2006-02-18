#	$NetBSD: Makefile.inc,v 1.8 2006/02/18 22:54:51 matt Exp $

.if ${OBJECT_FMT} == "ELF"
SRCS+=	__longjmp14.c
.endif
SRCS+=	__sigaction14_sigtramp.c __sigtramp3.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__
