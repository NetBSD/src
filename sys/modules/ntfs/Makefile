#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:11 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ntfs

KMOD=	ntfs

SRCS=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c \
	ntfs_conv.c

.include <bsd.kmodule.mk>
