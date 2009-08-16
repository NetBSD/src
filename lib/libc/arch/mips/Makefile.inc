#	$NetBSD: Makefile.inc,v 1.8.34.1 2009/08/16 03:36:02 matt Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__
