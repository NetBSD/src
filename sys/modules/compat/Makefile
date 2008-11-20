#	$NetBSD: Makefile,v 1.3.2.2 2008/11/20 22:18:31 christos Exp $

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
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_10
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_11
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_12
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_13
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_14
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_15
.endif
.if (${MACHINE_ARCH} != "x86_64")
CPPFLAGS+=	-DCOMPAT_16
.endif

CPPFLAGS+=	-DCOMPAT_20
CPPFLAGS+=	-DCOMPAT_30 -DCOMPAT_40 -DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DSYSVSEM -DSYSVSHM -DSYSVMSG -DCOMPAT_43

SRCS=	kern_exit_43.c kern_info_09.c
SRCS+=	kern_info_43.c kern_ipc_10.c kern_resource_43.c kern_sig_13.c
SRCS+=	kern_sig_43.c kern_xxx_12.c sysv_msg_14.c
SRCS+=	sysv_sem_14.c sysv_shm_14.c tty_43.c uipc_syscalls_30.c
SRCS+=	uipc_syscalls_43.c vfs_syscalls_12.c kern_sig_16.c
SRCS+=	vfs_syscalls_20.c vfs_syscalls_30.c vfs_syscalls_40.c
SRCS+=	vfs_syscalls_43.c vm_12.c vm_43.c compat_mod.c

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
