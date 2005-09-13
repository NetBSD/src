#	$NetBSD: Makefile.inc,v 1.1 2005/09/13 01:44:09 christos Exp $

.PATH: ${.CURDIR}/compat/sys
SRCS+= compat_getdents.c compat_getdirentries.c compat_msync.c \
    compat_semctl.c compat_sigaltstack.c compat_stat.c compat___stat13.c \
    compat_statfs.c
MAN+=	getdirentries.3
