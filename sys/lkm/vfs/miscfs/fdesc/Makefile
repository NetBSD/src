#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:32 explorer Exp $

.PATH:	/sys/miscfs/fdesc ${.CURDIR}/../..

NOMAN=	yep

KMOD=		fdesc
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
