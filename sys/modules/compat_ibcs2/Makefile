#	$NetBSD: Makefile,v 1.3.24.1 2017/12/03 11:38:51 jdolecek Exp $

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_ibcs2

CPPFLAGS+=	-DSYSVSHM -DSYSVSEM -DSYSVMSG
CPPFLAGS+=	-DCOMPAT_IBCS2

.PATH:	${S}/compat/ibcs2

SRCS+=	ibcs2_errno.c ibcs2_exec.c ibcs2_exec_coff.c ibcs2_exec_elf32.c
SRCS+=	ibcs2_exec_xout.c ibcs2_fcntl.c ibcs2_ioctl.c ibcs2_ipc.c
SRCS+=	ibcs2_misc.c ibcs2_signal.c ibcs2_signo.c ibcs2_socksys.c
SRCS+=	ibcs2_stat.c ibcs2_syscalls.c ibcs2_sysent.c ibcs2_mod.c

.include <bsd.kmodule.mk>
