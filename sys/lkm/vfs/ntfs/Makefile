#	$NetBSD: Makefile,v 1.4 2001/12/12 12:06:50 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
