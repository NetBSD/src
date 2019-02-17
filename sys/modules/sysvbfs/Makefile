#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

WARNS=	3

.include <bsd.kmodule.mk>
