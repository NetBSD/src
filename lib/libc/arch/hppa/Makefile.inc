#	$NetBSD: Makefile.inc,v 1.11 2021/10/31 22:24:34 thorpej Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

SRCS+=	__sigtramp2.S

SRCS+= bcopy.c

CPPFLAGS+= -I.
