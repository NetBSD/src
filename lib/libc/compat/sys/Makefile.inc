#	$NetBSD: Makefile.inc,v 1.4 2006/06/26 21:23:56 mrg Exp $

.PATH: ${COMPATDIR}/sys
SRCS+= compat_getdents.c compat_getdirentries.c compat_msync.c \
    compat_ntp_gettime.c \
    compat_semctl.c compat_sigaltstack.c compat_stat.c compat___stat13.c \
    compat_statfs.c compat_socket.c
MAN+=	getdirentries.3

.if ${MKLINT} != "no"
LSRCS+= Lint_Ovfork.c
.endif
