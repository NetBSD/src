#	$NetBSD: Makefile,v 1.1.4.1 2001/03/12 13:31:43 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

MKMAN=	no

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
