#	$NetBSD: Makefile,v 1.1.2.1 1999/06/21 01:26:05 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

MKMAN=	no

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c

.include <bsd.kmod.mk>
