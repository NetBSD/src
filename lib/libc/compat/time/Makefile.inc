#	$NetBSD: Makefile.inc,v 1.3 2023/09/16 18:40:26 christos Exp $

.PATH: ${COMPATDIR}/time
SRCS+= compat_asctime.c compat_localtime.c compat_difftime.c
