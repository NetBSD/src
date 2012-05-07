#	$NetBSD: Makefile.inc,v 1.2.46.1 2012/05/07 16:47:22 riz Exp $

.PATH: ${COMPATDIR}/stdlib
CPPFLAGS+=-I${COMPATDIR}/stdlib -I${COMPATDIR}/../stdlib
SRCS+=compat_putenv.c compat_unsetenv.c
