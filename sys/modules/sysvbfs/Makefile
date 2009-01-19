#	$NetBSD: Makefile,v 1.1.4.2 2009/01/19 13:20:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.kmodule.mk>
