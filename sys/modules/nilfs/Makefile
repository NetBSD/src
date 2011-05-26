#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:31 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
