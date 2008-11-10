#	$NetBSD: Makefile.inc,v 1.1.2.2 2008/11/10 00:13:02 christos Exp $

.PATH: ${COMPATDIR}/time
SRCS+= compat_localtime.c compat_difftime.c
