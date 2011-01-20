#	$NetBSD: Makefile.inc,v 1.1.40.1 2011/01/20 14:24:50 bouyer Exp $

SRCS+=compat_Ovfork.S compat___semctl.S compat___sigreturn14.S \
    compat_msgctl.S compat_shmctl.S compat_sigaction.S \
    compat_sigpending.S compat_sigprocmask.S compat_sigreturn.S \
    compat_sigsuspend.S compat_quotactl.S
