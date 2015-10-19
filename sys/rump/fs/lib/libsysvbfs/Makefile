#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:36 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../fs/sysvbfs

LIB=	rumpfs_sysvbfs
COMMENT=System V boot file system

SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
