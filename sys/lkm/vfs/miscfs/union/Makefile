#	$NetBSD: Makefile,v 1.1 1996/08/22 20:19:00 explorer Exp $

.PATH:	/sys/miscfs/union ${.CURDIR}/../..

NOMAN=	yep

KMOD=		union
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
