#	$NetBSD: Makefile,v 1.3.6.1 2018/04/12 13:42:48 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/procfs ${S} ${S}/arch/${MACHINE}/${MACHINE} \
	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
.PATH:  ${S}/arch/x86/x86
.endif

KMOD=	procfs
SRCS=	procfs_note.c procfs_status.c procfs_subr.c \
	procfs_vfsops.c procfs_vnops.c procfs_cmdline.c procfs_linux.c \
	procfs_machdep.c procfs_map.c procfs_regs.c procfs_fpregs.c \
	procfs_mem.c procfs_fd.c procfs_auxv.c

.include <bsd.kmodule.mk>
