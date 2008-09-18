#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:59 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdosfs
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmodule.mk>
