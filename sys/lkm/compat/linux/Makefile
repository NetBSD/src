#	$NetBSD: Makefile,v 1.31.10.1 2007/07/11 20:10:35 mjf Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}
.PATH:	$S/compat/linux/common $S/compat/linux
.PATH:	$S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+=	-DNWSDISPLAY=1 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_LINUX
CPPFLAGS+=	-nostdinc -DKTRACE -DNSEQUENCER=1 -DINET6
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S} -DCOMPAT_LINUX -D_LKM

KMOD=	compat_linux

# common source
SRCS+=	lkminit_emul.c
SRCS+=	linux_cdrom.c linux_errno.c linux_exec.c linux_file.c \
	linux_ioctl.c linux_ipc.c linux_misc.c linux_pipe.c \
	linux_sched.c linux_sigaction.c linux_signal.c linux_signo.c \
	linux_socket.c linux_termios.c linux_time.c linux_uselib.c
SRCS+=	linux_machdep.c linux_syscalls.c linux_sysent.c
SRCS+=	linux_fdio.c linux_hdio.c linux_blkio.c linux_sysctl.c
SRCS+=	linux_sg.c linux_mtio.c

# XXXX - temporary, not applicable for kernel with COMPAT_43, OSSAUDIO
#.PATH: $S/compat/common $S/compat/ossaudio
#SRCS+=	vfs_syscalls_43.c kern_resource_43.c kern_info_43.c ossaudio.c

# architecture or machine dependant files
.if ${MACHINE} == "i386"
.PATH:	$S/arch/x86/x86
SRCS+=	linux_break.c linux_llseek.c linux_misc_notalpha.c linux_oldolduname.c \
	linux_oldselect.c linux_sig_notalpha.c linux_socketcall.c \
	linux_ipccall.c linux_oldmmap.c linux_olduname.c linux_file64.c
SRCS+=	linux_syscall.c linux_trap.c
SRCS+=	linux_sigcode.S
.endif

.if ${MACHINE_ARCH} == "m68k"
SRCS+=	linux_ipccall.c linux_llseek.c linux_misc_notalpha.c		\
	linux_oldselect.c linux_sig_notalpha.c 	\
	linux_socketcall.c linux_oldmmap.c linux_file64.c
SRCS+=	linux_sig_machdep.S
.endif

.if ${MACHINE_ARCH} == "alpha"
CPPFLAGS+=	-Dalpha
SRCS+=	linux_olduname.c linux_syscall.c
SRCS+=	linux_sigcode.s
.endif

.if ${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=	-Dpowerpc
SRCS+=	linux_ipccall.c linux_misc_notalpha.c linux_sig_notalpha.c	\
	linux_socketcall.c linux_llseek.c linux_olduname.c linux_file64.c
SRCS+=	linux_exec_powerpc.c linux_sp_wrap_powerpc.S
SRCS+=	linux_sigcode.S linux_trap.c
.endif

.include <bsd.kmod.mk>
