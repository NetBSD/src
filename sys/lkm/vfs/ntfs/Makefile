#	$NetBSD: Makefile,v 1.2.2.2 2002/12/29 20:55:03 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/ntfs ${.CURDIR}/..

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
