#	$NetBSD: Makefile,v 1.2.4.1 2002/01/10 20:01:27 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/ntfs ${.CURDIR}/..

KMOD=	ntfs

SRCS=	lkminit_vfs.c
SRCS+=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c
SRCS+=	ntfs_conv.c

.include <bsd.kmod.mk>
