#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:36 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ptyfs

LIB=	rumpfs_ptyfs
COMMENT=/dev/pts pseudo file system

SRCS+=	ptyfs_subr.c ptyfs_vfsops.c ptyfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
