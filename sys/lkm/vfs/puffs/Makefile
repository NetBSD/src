#	$NetBSD: Makefile,v 1.1.2.2 2006/11/18 21:39:28 ad Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmod.mk>
