#	$NetBSD: Makefile,v 1.6 2019/02/17 04:05:45 rin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_freebsd

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DKTRACE -DPTRACE -DNTP
CPPFLAGS+=	-DCOMPAT_FREEBSD

.PATH:	${S}/compat/freebsd

SRCS+=	freebsd_exec.c freebsd_exec_elf32.c
SRCS+=	freebsd_file.c freebsd_fork.c freebsd_ioctl.c freebsd_ipc.c
SRCS+=	freebsd_misc.c freebsd_ptrace.c freebsd_sched.c freebsd_syscalls.c
SRCS+=	freebsd_sysctl.c freebsd_sysent.c freebsd_mod.c

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-DEXEC_ELF32
SRCS+=	freebsd_machdep.c freebsd_syscall.c freebsd_sigcode.S
.endif

WARNS=	3

.include <bsd.kmodule.mk>
