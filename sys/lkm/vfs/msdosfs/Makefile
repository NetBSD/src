#	$NetBSD: Makefile,v 1.5 1998/03/28 23:58:45 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>
