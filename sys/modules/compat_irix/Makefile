#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_irix

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVIPC -DCOMPAT_IRIX -DEXEC_ELF32

.PATH:	${S}/arch/mips/mips
SRCS+=	irix_syscall.c

.PATH:	${S}/compat/irix
SRCS+=	irix_dirent.c irix_errno.c irix_exec.c
SRCS+=	irix_exec_elf32.c irix_fcntl.c irix_ioctl.c
SRCS+=	irix_kmem.c irix_mman.c irix_misc.c
SRCS+=	irix_mod.c irix_mount.c irix_prctl.c
SRCS+=	irix_resource.c irix_signal.c irix_stat.c
SRCS+=	irix_swap.c irix_syscalls.c irix_sysctl.c
SRCS+=	irix_sysent.c irix_syssgi.c irix_sysmp.c
SRCS+=	irix_systeminfo.c irix_usema.c

.include <bsd.kmodule.mk>
