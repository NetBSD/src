#	$NetBSD: Makefile,v 1.8 1999/02/12 05:41:59 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../..

MKMAN=	no

KMOD=		procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c

.include <bsd.kmod.mk>
