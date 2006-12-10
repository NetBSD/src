#	$NetBSD: Makefile,v 1.2.2.2 2006/12/10 07:18:59 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_msgif.c puffs_subr.c puffs_transport.c puffs_vfsops.c	\
	puffs_vnops.c

.include <bsd.kmod.mk>
