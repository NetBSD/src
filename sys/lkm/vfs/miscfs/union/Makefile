#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:24 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

NOMAN=	yep

KMOD=		union
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
