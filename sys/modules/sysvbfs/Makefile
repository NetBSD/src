#	$NetBSD: Makefile,v 1.1.2.2 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/sysvbfs

KMOD=	sysvbfs
SRCS=	bfs.c bfs_sysvbfs.c sysvbfs.c sysvbfs_vnops.c sysvbfs_vfsops.c

.include <bsd.kmodule.mk>
