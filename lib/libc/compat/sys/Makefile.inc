#	$NetBSD: Makefile.inc,v 1.7.20.1 2008/11/08 21:45:38 christos Exp $

.PATH: ${COMPATDIR}/sys
SRCS+= compat_getdents.c compat_getdirentries.c compat_msync.c \
    compat_ntp_gettime.c \
    compat_semctl.c compat_sigaltstack.c compat_stat.c compat___stat13.c \
    compat_statfs.c compat_socket.c compat_getfh.c \
    compat_fhopen.c compat___fhstat30.c compat_fhstatvfs.c compat_fhstatvfs1.c \
    compat_mount.c compat_getrusage.c compat_gettimeofday.c compat_utimes.c \
    compat_wait4.c compat_sigtimedwait.c compat_settimeofday.c compat_timer.c \
    compat_clock.c compat_itimer.c compat_select.c compat_nanosleep.c
MAN+=	getdirentries.3

.if ${MKLINT} != "no"
LSRCS+= Lint_Ovfork.c
.endif
