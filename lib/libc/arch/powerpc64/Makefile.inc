#	$NetBSD: Makefile.inc,v 1.4 2021/10/31 22:24:35 thorpej Exp $

KMINCLUDES=
KMSRCS=	

CPPFLAGS+=	-I.

SRCS+=		__sigtramp2.S
