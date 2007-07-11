#	$NetBSD: Makefile,v 1.20.14.1 2007/07/11 20:10:35 mjf Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_FREEBSD -DKTRACE
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

KMOD=	compat_freebsd

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	freebsd_exec.c freebsd_file.c freebsd_fork.c \
	freebsd_ioctl.c freebsd_ipc.c freebsd_misc.c \
	freebsd_sched.c freebsd_syscalls.c \
	freebsd_sysctl.c freebsd_sysent.c

# XXXX - temporary, not applicable for kernel with COMPAT_43, OSSAUDIO
#.PATH: $S/compat/common $S/compat/ossaudio
#SRCS+=	uipc_syscalls_43.c vfs_syscalls_43.c kern_resource_43.c \
#	kern_info_43.c ossaudio.c
#SRCS+=	kern_info_09.c kern_sig_43.c kern_sig_13.c sysv_msg_14.c vm_43.c \
#	sysv_shm_14.c kern_exit_43.c vm_12.c vfs_syscalls_12.c sysv_sem_14.c \
#	sysv_ipc_14.c

# MD source files
.if ${MACHINE} == "i386"
.PATH: $S/arch/i386/i386
SRCS+=	freebsd_machdep.c freebsd_sigcode.S freebsd_syscall.c
.endif

.include <bsd.kmod.mk>
