#	$NetBSD: Makefile,v 1.11 2001/01/17 00:09:31 fvdl Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE}

MKMAN=	no

KMOD=	procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c procfs_cmdline.c procfs_linux.c
SRCS+=	procfs_machdep.c

.include <bsd.kmod.mk>
