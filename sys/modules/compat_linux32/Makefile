#	$NetBSD: Makefile,v 1.9 2019/02/17 04:05:45 rin Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_linux32

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG -DCOMPAT_LINUX
CPPFLAGS+=	-DCOMPAT_LINUX32 -DCOMPAT_NETBSD32
CPPFLAGS+=      -I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/linux32/common
SRCS+=	linux32_dirent.c linux32_exec.c linux32_exec_elf32.c linux32_fcntl.c
SRCS+=	linux32_ioctl.c linux32_misc.c linux32_mman.c
SRCS+=	linux32_mod.c linux32_resource.c linux32_sched.c linux32_signal.c
SRCS+=	linux32_socket.c linux32_socketcall.c linux32_stat.c linux32_sysctl.c
SRCS+=	linux32_sysinfo.c linux32_termios.c linux32_time.c
SRCS+=	linux32_unistd.c linux32_utsname.c linux32_wait.c

.if ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DEXEC_ELF32 -DLINUX32_DEBUGLINK_SIGNATURE
.PATH:	${S}/compat/linux32/arch/amd64
.PATH:	${S}/arch/amd64/amd64
.PATH:	${S}/arch/x86/x86
SRCS+=	linux32_ipccall.c linux32_machdep.c linux32_missing.c
SRCS+=	linux32_syscalls.c linux32_sysent.c linux32_sigcode.S
SRCS+=	linux32_syscall.c linux32_uid16.c
.PATH:	${S}/compat/linux/common
SRCS+=	linux_oldmmap.c linux_exec_elf32.c
.endif

WARNS=	3

.include <bsd.kmodule.mk>
