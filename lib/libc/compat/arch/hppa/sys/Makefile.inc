#	$NetBSD: Makefile.inc,v 1.3 2021/10/30 21:39:28 christos Exp $

SRCS+=compat_Ovfork.S compat___semctl.S compat___sigreturn14.S \
    compat_msgctl.S compat_shmctl.S compat_sigaction.S \
    compat_sigpending.S compat_sigprocmask.S compat_sigreturn.S \
    compat_sigsuspend.S compat_quotactl.S compat___sigtramp1.S
