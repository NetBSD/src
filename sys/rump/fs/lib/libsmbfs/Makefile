#	$NetBSD: Makefile,v 1.1 2009/09/04 12:16:54 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/smbfs

LIB=	rumpfs_smbfs

SRCS=	smbfs_io.c smbfs_kq.c smbfs_node.c smbfs_smb.c smbfs_subr.c	\
	smbfs_vfsops.c smbfs_vnops.c

CFLAGS+=        -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
