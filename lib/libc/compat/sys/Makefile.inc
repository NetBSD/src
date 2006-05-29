#	$NetBSD: Makefile.inc,v 1.3 2006/05/29 10:02:56 drochner Exp $

.PATH: ${COMPATDIR}/sys
SRCS+= compat_getdents.c compat_getdirentries.c compat_msync.c \
    compat_ntp_gettime.c \
    compat_semctl.c compat_sigaltstack.c compat_stat.c compat___stat13.c \
    compat_statfs.c
MAN+=	getdirentries.3

.if ${MKLINT} != "no"
LSRCS+= Lint_Ovfork.c
.endif
