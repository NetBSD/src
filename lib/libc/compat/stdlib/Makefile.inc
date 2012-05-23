#	$NetBSD: Makefile.inc,v 1.2.44.1 2012/05/23 10:07:30 yamt Exp $

.PATH: ${COMPATDIR}/stdlib
CPPFLAGS+=-I${COMPATDIR}/stdlib -I${COMPATDIR}/../stdlib
SRCS+=compat_putenv.c compat_unsetenv.c
