#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:17 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ntfs

KMOD=	ntfs

SRCS=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c \
	ntfs_conv.c

.include <bsd.kmodule.mk>
