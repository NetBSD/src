#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:55 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
