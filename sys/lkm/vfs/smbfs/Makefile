#	$NetBSD: Makefile,v 1.1 2003/04/02 09:45:43 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/smbfs

KMOD=	smbfs

SRCS=	lkminit_vfs.c
SRCS+=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c \
	smbfs_vfsops.c smbfs_vnops.c

.include <bsd.kmod.mk>
