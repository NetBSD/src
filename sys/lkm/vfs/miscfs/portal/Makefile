#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:47 explorer Exp $

.PATH:	/sys/miscfs/portal ${.CURDIR}/../..

NOMAN=	yep

KMOD=		portal
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
