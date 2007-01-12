#	$NetBSD: Makefile,v 1.1.2.3 2007/01/12 01:04:10 ad Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_msgif.c puffs_subr.c puffs_transport.c puffs_vfsops.c	\
	puffs_vnops.c

.include <bsd.kmod.mk>
