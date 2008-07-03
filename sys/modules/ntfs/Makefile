#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:20 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/ntfs

KMOD=	ntfs

SRCS=	ntfs_compr.c ntfs_ihash.c ntfs_subr.c ntfs_vfsops.c ntfs_vnops.c \
	ntfs_conv.c

.include <bsd.kmodule.mk>
