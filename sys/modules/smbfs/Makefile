#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:13 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/smbfs

KMOD=	smbfs
SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c \
	smbfs_vfsops.c smbfs_vnops.c

.include <bsd.kmodule.mk>
