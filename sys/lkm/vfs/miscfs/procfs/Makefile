#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:51 explorer Exp $

.PATH:	/sys/miscfs/procfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		procfs
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	procfs_vfsops.c procfs_vnops.c procfs_ctl.c procfs_note.c
SRCS+=	procfs_status.c procfs_subr.c

.include <bsd.kmod.mk>
