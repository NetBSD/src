#	$NetBSD: Makefile,v 1.12.2.1 2001/08/24 00:11:56 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE} \
	$S/arch/${MACHINE_CPU}/${MACHINE_CPU}

MKMAN=	no

KMOD=	procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c procfs_cmdline.c procfs_linux.c
SRCS+=	procfs_machdep.c procfs_map.c

.include <bsd.kmod.mk>
