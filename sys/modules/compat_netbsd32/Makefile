#	$NetBSD: Makefile,v 1.11.10.5 2016/10/05 20:56:04 skrll Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32

CPPFLAGS+=	-DCOMPAT_09
CPPFLAGS+=	-DCOMPAT_10
CPPFLAGS+=	-DCOMPAT_11
CPPFLAGS+=	-DCOMPAT_12
CPPFLAGS+=	-DCOMPAT_13
CPPFLAGS+=	-DCOMPAT_14
CPPFLAGS+=	-DCOMPAT_15
CPPFLAGS+=	-DCOMPAT_16

CPPFLAGS+=	-DCOMPAT_20
CPPFLAGS+=	-DCOMPAT_30 -DCOMPAT_40 -DCOMPAT_50
CPPFLAGS+=	-DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80
CPPFLAGS+=	-DCOMPAT_43
CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DEXEC_ELF32 -DEXEC_ELF64
CPPFLAGS+=	-DCOREDUMP -DNTP -DVMSWAP
CPPFLAGS+=	-DNTP -DKTRACE

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_compat_09.c netbsd32_compat_10.c
SRCS+=	netbsd32_compat_12.c netbsd32_compat_13.c
SRCS+=	netbsd32_compat_14.c netbsd32_compat_20.c
SRCS+=	netbsd32_compat_30.c netbsd32_compat_43.c
SRCS+=	netbsd32_compat_50.c netbsd32_compat_60.c
SRCS+=	netbsd32_core.c netbsd32_event.c
SRCS+=	netbsd32_exec_elf32.c
SRCS+=	netbsd32_execve.c netbsd32_fs.c
SRCS+=	netbsd32_ioctl.c
SRCS+=	netbsd32_lwp.c netbsd32_netbsd.c
SRCS+=	netbsd32_select.c
SRCS+=	netbsd32_sem.c netbsd32_signal.c
SRCS+=	netbsd32_socket.c netbsd32_syscalls.c
SRCS+=	netbsd32_sysctl.c netbsd32_sysent.c
SRCS+=	netbsd32_time.c netbsd32_wait.c
SRCS+=	netbsd32_mod.c netbsd32_module.c

.if ${MACHINE_CPU} != "mips" && ${MACHINE_CPU} != "powerpc"
CPPFLAGS+=	-DEXEC_AOUT -DCOMPAT_NOMID
SRCS+=	netbsd32_exec_aout.c
.endif

.if ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/amd64/amd64
SRCS+=	netbsd32_machdep.c netbsd32_sigcode.S netbsd32_syscall.c
.endif

.if ${MACHINE_ARCH} == "sparc64"
.PATH:	${S}/arch/sparc64/sparc64
SRCS+=	netbsd32_machdep.c netbsd32_sigcode.S
.endif

.include <bsd.kmodule.mk>
