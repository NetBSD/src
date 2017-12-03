#	$NetBSD: Makefile,v 1.3.14.1 2017/12/03 11:39:12 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/nilfs

LIB=	rumpfs_nilfs
COMMENT=NILFS

SRCS=	nilfs_subr.c nilfs_vfsops.c nilfs_vnops.c

CWARNFLAGS.clang+=	-Wno-self-assign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
