#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:56 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/ffs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		ffs

SRCS=	lkminit_vfs.c
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c
SRCS+=	ffs_vfsops.c ffs_vnops.c

.include <bsd.kmod.mk>
