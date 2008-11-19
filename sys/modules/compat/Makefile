#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:07 ad Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat

CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG -DCOMPAT_43
CPPFLAGS+=	-DCOMPAT_09 -DCOMPAT_10 -DCOMPAT_11
CPPFLAGS+=	-DCOMPAT_12 -DCOMPAT_13 -DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_15 -DCOMPAT_16	-DCOMPAT_20
CPPFLAGS+=	-DCOMPAT_30 -DCOMPAT_40 -DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS=	compat_exec.c kern_exit_43.c kern_info_09.c
SRCS+=	kern_info_43.c kern_ipc_10.c kern_resource_43.c kern_sig_13.c
SRCS+=	kern_sig_43.c kern_xxx_12.c sysv_ipc_14.c sysv_msg_14.c
SRCS+=	sysv_sem_14.c sysv_shm_14.c tty_43.c uipc_syscalls_30.c
SRCS+=	uipc_syscalls_43.c vfs_syscalls_12.c kern_sig_16.c
SRCS+=	vfs_syscalls_20.c vfs_syscalls_30.c vfs_syscalls_40.c
SRCS+=	vfs_syscalls_43.c vm_12.c vm_43.c compat_mod.c

.if ${MACHINE} == "i386"
.PATH:	${S}/arch/i386/i386
SRCS+=	compat_13_machdep.c compat_16_machdep.c
.endif

.include <bsd.kmodule.mk>
