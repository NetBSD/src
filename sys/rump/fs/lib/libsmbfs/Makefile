#	$NetBSD: Makefile,v 1.1.2.2 2009/09/16 13:38:04 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/smbfs

LIB=	rumpfs_smbfs

SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c	\
	smbfs_vfsops.c smbfs_vnops.c

CFLAGS+=        -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
