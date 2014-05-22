#	$NetBSD: Makefile.inc,v 1.9.6.1 2014/05/22 11:36:47 yamt Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S

SRCS+= bcopy.c

CPPFLAGS+= -I.
