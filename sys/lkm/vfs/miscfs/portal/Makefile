#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:19 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

NOMAN=	yep

KMOD=		portal
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
