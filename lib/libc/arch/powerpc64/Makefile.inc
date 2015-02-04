#	$NetBSD: Makefile.inc,v 1.2.64.1 2015/02/04 22:00:24 martin Exp $

KMINCLUDES=
KMSRCS=	

CPPFLAGS+=	-I.

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S
