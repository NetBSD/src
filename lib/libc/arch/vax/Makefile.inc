#	$NetBSD: Makefile.inc,v 1.10 2021/10/31 22:24:36 thorpej Exp $

SRCS+=	__longjmp14.c
SRCS+=	__sigtramp3.S

CPPFLAGS+= -I.
CPPFLAGS.assym.h+=-D__LIBC12_SOURCE__
