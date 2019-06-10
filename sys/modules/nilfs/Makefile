#	$NetBSD: Makefile,v 1.2.56.1 2019/06/10 22:09:34 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
