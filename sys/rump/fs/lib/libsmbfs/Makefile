#	$NetBSD: Makefile,v 1.1.2.3 2010/03/11 15:04:36 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/smbfs

LIB=	rumpfs_smbfs

SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c	\
	smbfs_vfsops.c smbfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
