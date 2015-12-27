#	$NetBSD: Makefile,v 1.1.42.1 2015/12/27 12:10:14 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ptyfs

LIB=	rumpfs_ptyfs
COMMENT=/dev/pts pseudo file system

SRCS+=	ptyfs_subr.c ptyfs_vfsops.c ptyfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
