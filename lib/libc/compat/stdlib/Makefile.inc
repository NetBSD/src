#	$NetBSD: Makefile.inc,v 1.3.2.1 2014/08/20 00:02:14 tls Exp $

.PATH: ${COMPATDIR}/stdlib
CPPFLAGS+=-I${COMPATDIR}/stdlib -I${COMPATDIR}/../stdlib
SRCS+=compat_putenv.c compat_random.c compat_unsetenv.c
