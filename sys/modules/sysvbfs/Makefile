#	$NetBSD: Makefile,v 1.1 2009/01/05 15:36:51 pooka Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.kmodule.mk>
