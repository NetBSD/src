#	$NetBSD: Makefile,v 1.5 2002/12/23 17:38:21 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/ntfs ${.CURDIR}/..

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
