#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:18 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		nullfs
VFS_LKM=	yes

#
# Drat.  These aren't standard...
#
MODVNOPS=null_vnodeop_opv_desc
MODVFSOPS=null_vfsops

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c null_subr.c

.include <bsd.kmod.mk>
