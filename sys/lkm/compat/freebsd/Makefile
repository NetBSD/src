#	$NetBSD: Makefile,v 1.9 2000/12/17 16:20:23 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_FREEBSD -DKTRACE
CPPFLAGS+=	-nostdinc
# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=freebsd_emul_find				\
		-Dstackgap_init=freebsd_stackgap_init			\
		-Demul_flags_translate=freebsd_emul_flags_translate	\
		-Dstackgap_alloc=freebsd_stackgap_alloc		 	\
		-Dcompat_offseterr=freebsd_compat_offseterr
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE \
		-I. -I${S}

MKMAN=	no

KMOD=	compat_freebsd

# common source files
SRCS=	lkminit_emul.c
SRCS+=	freebsd_exec.c freebsd_file.c freebsd_ioctl.c freebsd_ipc.c \
	freebsd_misc.c freebsd_ptrace.c freebsd_syscalls.c freebsd_sysent.c

# emul_find() and friends - symbols are redefined to freebsd_FOO()
.PATH: $S/compat/common
SRCS+=	compat_util.c

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
SRCS+=	freebsd_machdep.c freebsd_sigcode.s
.endif

.include <bsd.kmod.mk>
