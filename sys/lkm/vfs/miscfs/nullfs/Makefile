#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:44 explorer Exp $

.PATH:	/sys/miscfs/nullfs ${.CURDIR}/../..

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
