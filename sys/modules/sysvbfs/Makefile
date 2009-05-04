#	$NetBSD: Makefile,v 1.1.8.2 2009/05/04 08:14:13 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.kmodule.mk>
