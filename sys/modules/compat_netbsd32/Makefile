#	$NetBSD: Makefile,v 1.24 2019/03/23 03:04:57 pgoyette Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DEXEC_ELF32 -DEXEC_ELF64
CPPFLAGS+=	-DCOREDUMP -DNTP -DVMSWAP
CPPFLAGS+=	-DKTRACE -DMODULAR
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_core.c		netbsd32_event.c
SRCS+=	netbsd32_exec_elf32.c	netbsd32_execve.c
SRCS+=	netbsd32_fd.c		netbsd32_fs.c
SRCS+=	netbsd32_ioctl.c	netbsd32_kern_proc.c
SRCS+=	netbsd32_lwp.c		netbsd32_mod.c
SRCS+=	netbsd32_netbsd.c	netbsd32_quota.c
SRCS+=	netbsd32_rlimit.c	netbsd32_select.c
SRCS+=	netbsd32_sem.c		netbsd32_signal.c
SRCS+=	netbsd32_socket.c	netbsd32_syscalls.c
SRCS+=	netbsd32_sysctl.c	netbsd32_sysent.c
SRCS+=	netbsd32_time.c		netbsd32_wait.c	
SRCS+=	netbsd32_vm.c
SRCS+=	netbsd32_module.c

.if ${MACHINE_CPU} != "mips" && ${MACHINE_CPU} != "powerpc"
CPPFLAGS+=	-DEXEC_AOUT -DCOMPAT_NOMID
SRCS+=	netbsd32_exec_aout.c
.endif

.if ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/amd64/amd64
CPPFLAGS+=	-DMTRR
SRCS+=	netbsd32_syscall.c
SRCS+=	netbsd32_machdep.c
.endif

.if ${MACHINE_ARCH} == "sparc64"
.PATH:	${S}/arch/sparc64/sparc64
SRCS+=	netbsd32_sigcode.S
SRCS+=	netbsd32_machdep.c
.endif

.if ${MACHINE_ARCH} == "arm"
.PATH:	${S}/arch/arm/arm32
SRCS+=	netbsd32_machdep.c
.endif

WARNS=	3

.include <bsd.kmodule.mk>
