#	$NetBSD: Makefile,v 1.10 2000/12/12 19:06:53 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/linux/arch/${MACHINE_ARCH}
.PATH:	$S/compat/linux/common $S/compat/linux
.PATH:	$S/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CPPFLAGS+=	-DNWSDISPLAY=1 -DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_LINUX
CPPFLAGS+=	-nostdinc -DKTRACE -DNSEQUENCER=1
# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=linux_emul_find -Dstackgap_init=linux_stackgap_init\
		-Demul_flags_translate=linux_emul_flags_translate \
		-Dstackgap_alloc=linux_stackgap_alloc \
		-Dcompat_offseterr=linux_compat_offseterr
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S} -DCOMPAT_LINUX -D_LKM

MKMAN=	no

KMOD=	compat_linux

# common source
SRCS+=	lkminit_emul.c
SRCS+=	linux_cdrom.c linux_errno.c linux_exec.c linux_file.c \
	linux_ioctl.c linux_ipc.c linux_misc.c linux_pipe.c \
	linux_sched.c linux_sigaction.c linux_sigarray.c linux_signal.c \
	linux_socket.c linux_termios.c linux_uselib.c
SRCS+=	linux_machdep.c linux_syscalls.c linux_sysent.c
SRCS+=	linux_fdio.c linux_hdio.c

# emul_find() and friends - symbols are redefined to linux_FOO()
.PATH: $S/compat/common
SRCS+=	compat_util.c

# XXXX - temporary, not applicable for kernel with COMPAT_43, OSSAUDIO
#.PATH: $S/compat/common $S/compat/ossaudio
#SRCS+=	uipc_syscalls_43.c vfs_syscalls_43.c kern_resource_43.c \
#	kern_info_43.c ossaudio.c

# architecture or machine dependant files
.if ${MACHINE} == "i386"
SRCS+=	linux_break.c linux_llseek.c linux_misc_notalpha.c linux_oldolduname.c \
	linux_oldselect.c linux_sig_notalpha.c linux_socketcall.c \
	linux_ipccall.c linux_oldmmap.c linux_olduname.c
SRCS+=	linux_ptrace.c linux_syscall.c
SRCS+=	linux_sigcode.s
.endif

.if ${MACHINE_ARCH} == "m68k"
SRCS+=	linux_ipccall.c linux_llseek.c linux_misc_notalpha.c		\
	linux_oldselect.c linux_sig_notalpha.c 	\
	linux_socketcall.c linux_oldmmap.c linux_file64.c
SRCS+=	linux_sig_machdep.S
.endif

.if ${MACHINE_ARCH} == "alpha"
CPPFLAGS+=	-Dalpha
SRCS+=	linux_olduname.c
SRCS+=	linux_sigcode.s
.endif

.include <bsd.kmod.mk>
