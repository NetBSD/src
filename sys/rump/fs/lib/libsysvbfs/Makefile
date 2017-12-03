#	$NetBSD: Makefile,v 1.1.46.1 2017/12/03 11:39:13 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../fs/sysvbfs

LIB=	rumpfs_sysvbfs
COMMENT=System V boot file system

SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
