#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:12 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

NOMAN=	yep

KMOD=		fdesc
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
