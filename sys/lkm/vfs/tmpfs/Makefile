#	$NetBSD: Makefile,v 1.1.2.2 2006/11/18 21:39:28 ad Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/tmpfs ${.CURDIR}/..

KMOD=	tmpfs

SRCS=	lkminit_vfs.c
SRCS+=	tmpfs_fifoops.c tmpfs_pool.c tmpfs_specops.c tmpfs_subr.c
SRCS+=	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmod.mk>
