#	$NetBSD: Makefile.inc,v 1.6 2004/06/03 16:34:19 mhitch Exp $

.if ${OBJECT_FMT} == "ELF"
SRCS+=	__longjmp14.c
.endif
SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S __sigtramp3.S

CPPFLAGS+= -I.
