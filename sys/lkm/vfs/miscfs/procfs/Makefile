#	$NetBSD: Makefile,v 1.12.2.3 2002/06/20 03:47:55 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE} \
	$S/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=	procfs_vfsops.c procfs_vnops.c procfs_cmdline.c procfs_linux.c
SRCS+=	procfs_machdep.c procfs_map.c procfs_regs.c procfs_fpregs.c
SRCS+=	procfs_mem.c

.include <bsd.kmod.mk>
