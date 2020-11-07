#	$NetBSD: Makefile,v 1.37 2020/11/07 21:42:32 christos Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32

#.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
#NETBSD32_DRMKMS?=yes
#.endif

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DEXEC_ELF32 -DEXEC_ELF64
CPPFLAGS+=	-DNTP -DVMSWAP
CPPFLAGS+=	-DKTRACE -DMODULAR
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.if ${NETBSD32_DRMKMS:Uno} == "yes"
# disable for now
#CPPFLAGS+=	-DNETBSD32_DRMKMS
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/common/include
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/drm2/include
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/drm2/dist
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/drm2/dist/include
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/drm2/dist/uapi
.endif

.PATH:	${S}/compat/netbsd32
.if ${NETBSD32_DRMKMS:Uno} == "yes"
SRCS+=	netbsd32_drm.c
.endif
SRCS+=	netbsd32_acl.c		netbsd32_event.c
SRCS+=	netbsd32_exec_elf32.c	netbsd32_execve.c
SRCS+=	netbsd32_fd.c		netbsd32_fs.c
SRCS+=	netbsd32_futex.c
SRCS+=	netbsd32_ioctl.c	netbsd32_kern_proc.c
SRCS+=	netbsd32_lwp.c		netbsd32_mod.c
SRCS+=	netbsd32_module.c	netbsd32_netbsd.c
SRCS+=	netbsd32_rlimit.c	netbsd32_select.c
SRCS+=	netbsd32_sem.c		netbsd32_signal.c
SRCS+=	netbsd32_socket.c	netbsd32_syscalls.c
SRCS+=	netbsd32_sysctl.c	netbsd32_sysent.c
SRCS+=	netbsd32_time.c		netbsd32_wait.c	
SRCS+=	netbsd32_vm.c

.if ${MACHINE_CPU} != "mips" && ${MACHINE_CPU} != "powerpc" && \
    ${MACHINE_CPU} != "aarch64"
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

.if !empty(MACHINE_ARCH:M*arm*)
.PATH:	${S}/arch/arm/arm32
SRCS+=	netbsd32_machdep.c
.endif

.include <bsd.kmodule.mk>
