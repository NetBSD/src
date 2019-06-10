#	$NetBSD: Makefile,v 1.1.72.1 2019/06/10 22:09:40 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

WARNS=	3

.include <bsd.kmodule.mk>
