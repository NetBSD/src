#	$NetBSD: Makefile,v 1.6 2021/10/23 07:41:38 hannken Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos
COMMENT=FAT

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_rename.c msdosfs_vnops.c msdosfs_unicode.c

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
