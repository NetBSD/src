#	$NetBSD: Makefile,v 1.5 2001/06/06 21:33:24 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/netbsd32 $S/arch/${MACHINE}/${MACHINE}

CPPFLAGS+=	-DSYSVSEM -DSYSVMSG -DSYSVSHM -DCOMPAT_NETBSD32
CPPFLAGS+=	-DCOMPAT_10 -DCOMPAT_12 -DCOMPAT_13 -DCOMPAT_43
CPPFLAGS+=	-DEXEC_AOUT -DEXEC_ELF32 # for netbsd32_exec.c
CPPFLAGS+=	-nostdinc
AFLAGS=		-x assembler-with-cpp -traditional-cpp -D_LOCORE -D_KERNEL \
		-I. -I${S}

MKMAN=	no

KMOD=	compat_netbsd32

# common source files
SRCS+=	lkminit_emul.c
SRCS+=	netbsd32_compat_09.c netbsd32_compat_10.c netbsd32_compat_12.c \
	netbsd32_compat_13.c netbsd32_compat_14.c netbsd32_compat_43.c \
	netbsd32_netbsd.c netbsd32_execve.c netbsd32_fs.c netbsd32_ioctl.c \
	netbsd32_ipc.c netbsd32_select.c netbsd32_signal.c netbsd32_socket.c \
	netbsd32_sysctl.c netbsd32_time.c netbsd32_wait.c netbsd32_syscalls.c \
	netbsd32_sysent.c netbsd32_kern_sig.c netbsd32_uvm_unix.c

.if (${MACHINE} == "sparc64")
SRCS+=	netbsd32_machdep.c netbsd32_sigcode.s
.endif

.include <bsd.kmod.mk>
