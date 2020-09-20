#	$NetBSD: Makefile.inc,v 1.4 2020/09/20 20:14:50 christos Exp $

SRCS+=	compat_msgctl.S compat___semctl.S compat_shmctl.S compat_quotactl.S \
	compat_missing.c
