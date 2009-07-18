#	$NetBSD: Makefile,v 1.1 2009/07/18 16:31:43 reinoud Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/nilfs

KMOD=  	nilfs
SRCS=	nilfs_vfsops.c nilfs_vnops.c nilfs_subr.c

.include <bsd.kmodule.mk>
