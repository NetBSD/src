#	$NetBSD: Makefile,v 1.8 2013/04/16 15:21:33 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat

#
# We don't want to hook these in for the native emulation, but do want
# the symbols available for other emulations.  So include the source
# files but don't hook them in via compat_modcmd().
#
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_09
CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_11
CPPFLAGS+=	-DCOMPAT_12
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_15
CPPFLAGS+=	-DCOMPAT_16
.endif

CPPFLAGS+=	-DCOMPAT_20
CPPFLAGS+=	-DCOMPAT_30 -DCOMPAT_40 -DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG -DCOMPAT_43

SRCS=	kern_exit_43.c kern_info_09.c
SRCS+=	kern_info_43.c kern_ipc_10.c kern_resource_43.c kern_select_50.c
SRCS+=	kern_sig_13.c kern_sig_43.c kern_xxx_12.c sysv_msg_14.c
SRCS+=	sysv_sem_14.c sysv_shm_14.c tty_43.c uipc_syscalls_30.c
SRCS+=	uipc_syscalls_43.c vfs_syscalls_12.c kern_sig_16.c
SRCS+=	vfs_syscalls_20.c vfs_syscalls_30.c vfs_syscalls_40.c
SRCS+=	vfs_syscalls_43.c vm_12.c vm_43.c compat_mod.c
SRCS+=	sysv_msg_50.c sysv_sem_50.c kern_time_50.c
SRCS+=	sysv_shm_50.c vfs_syscalls_50.c sysv_ipc_50.c
SRCS+=	tty_60.c kern_time_60.c
SRCS+=	kern_time_60.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/compat_13_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/compat_13_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/compat_13_machdep.c))
SRCS+=	compat_13_machdep.c
.endif

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/compat_16_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/compat_16_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/compat_16_machdep.c))
SRCS+=	compat_16_machdep.c
.endif

.include <bsd.kmodule.mk>
