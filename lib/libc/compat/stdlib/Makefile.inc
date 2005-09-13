#	$NetBSD: Makefile.inc,v 1.1 2005/09/13 01:44:09 christos Exp $

.PATH: ${.CURDIR}/compat/stdlib
CPPFLAGS+=-I${.CURDIR}/stdlib
SRCS+=compat_unsetenv.c 
