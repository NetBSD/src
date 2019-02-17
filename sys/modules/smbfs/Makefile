#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:58 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/smbfs

KMOD=	smbfs
SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c \
	smbfs_vfsops.c smbfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
