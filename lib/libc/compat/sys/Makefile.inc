#	$NetBSD: Makefile.inc,v 1.7 2007/07/14 15:53:04 dsl Exp $

.PATH: ${COMPATDIR}/sys
SRCS+= compat_getdents.c compat_getdirentries.c compat_msync.c \
    compat_ntp_gettime.c \
    compat_semctl.c compat_sigaltstack.c compat_stat.c compat___stat13.c \
    compat_statfs.c compat_socket.c compat_getfh.c \
    compat_fhopen.c compat___fhstat30.c compat_fhstatvfs.c compat_fhstatvfs1.c \
    compat_mount.c
MAN+=	getdirentries.3

.if ${MKLINT} != "no"
LSRCS+= Lint_Ovfork.c
.endif
