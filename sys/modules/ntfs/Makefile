#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:55 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ntfs

KMOD=	ntfs

SRCS=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c \
	ntfs_conv.c

WARNS=	3

.include <bsd.kmodule.mk>
