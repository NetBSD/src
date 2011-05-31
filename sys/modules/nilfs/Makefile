#	$NetBSD: Makefile,v 1.1.8.1 2011/05/31 03:05:06 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
