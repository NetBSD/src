#	$NetBSD: Makefile,v 1.1.24.1 2017/12/03 11:39:13 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ptyfs

LIB=	rumpfs_ptyfs
COMMENT=/dev/pts pseudo file system

SRCS+=	ptyfs_subr.c ptyfs_vfsops.c ptyfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
