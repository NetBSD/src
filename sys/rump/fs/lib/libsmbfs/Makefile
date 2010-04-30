#	$NetBSD: Makefile,v 1.1.4.1 2010/04/30 14:44:28 uebayasi Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/smbfs

LIB=	rumpfs_smbfs

SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c	\
	smbfs_vfsops.c smbfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
