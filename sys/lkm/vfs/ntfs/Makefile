#	$NetBSD: Makefile,v 1.2.2.1 2002/01/08 00:33:28 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
