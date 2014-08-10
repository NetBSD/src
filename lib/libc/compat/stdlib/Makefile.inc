#	$NetBSD: Makefile.inc,v 1.3.8.1 2014/08/10 06:51:50 tls Exp $

.PATH: ${COMPATDIR}/stdlib
CPPFLAGS+=-I${COMPATDIR}/stdlib -I${COMPATDIR}/../stdlib
SRCS+=compat_putenv.c compat_random.c compat_unsetenv.c
