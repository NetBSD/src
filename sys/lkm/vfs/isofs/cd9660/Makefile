#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:25 explorer Exp $

.PATH:	/sys/isofs/cd9660 ${.CURDIR}/../..

NOMAN=	yep

KMOD=		cd9660
VFS_LKM=	yes

SRCS=	lkminit_vfs.c
SRCS+=	cd9660_bmap.c cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c
SRCS+=	cd9660_node.c cd9660_vnops.c cd9660_util.c

.include <bsd.kmod.mk>
