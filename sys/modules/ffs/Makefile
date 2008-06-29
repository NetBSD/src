#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:16 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
SRCS=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmodule.mk>
