#	$NetBSD: Makefile,v 1.2.6.1 2009/05/13 17:22:18 jym Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_freebsd

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DKTRACE -DPTRACE -DNTP
CPPFLAGS+=	-DCOMPAT_FREEBSD

.PATH:	${S}/compat/freebsd

SRCS+=	freebsd_exec.c freebsd_exec_aout.c freebsd_exec_elf32.c
SRCS+=	freebsd_file.c freebsd_fork.c freebsd_ioctl.c freebsd_ipc.c
SRCS+=	freebsd_misc.c freebsd_ptrace.c freebsd_sched.c freebsd_syscalls.c
SRCS+=	freebsd_sysctl.c freebsd_sysent.c freebsd_mod.c

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-DEXEC_ELF32 -DEXEC_AOUT
.PATH:	${S}/arch/i386/i386
SRCS+=	freebsd_machdep.c freebsd_syscall.c freebsd_sigcode.S
.endif

.include <bsd.kmodule.mk>
