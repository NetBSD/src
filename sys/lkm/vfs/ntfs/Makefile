#	$NetBSD: Makefile,v 1.1 1999/05/12 07:46:43 tron Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

MKMAN=	no

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c

.include <bsd.kmod.mk>
