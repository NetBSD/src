#	$NetBSD: Makefile,v 1.1.92.1 2019/06/10 22:09:35 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ntfs

KMOD=	ntfs

SRCS=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c \
	ntfs_conv.c

WARNS=	3

.include <bsd.kmodule.mk>
